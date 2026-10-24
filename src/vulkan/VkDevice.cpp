#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkMemory.h"

#include <cstring>

namespace slag::vk {

namespace {

constexpr uint32_t noFamily = UINT32_MAX;

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
    return alignment == 0 ? value : (value + alignment - 1) / alignment * alignment;
}

uint32_t findFamily(const GraphicsDeviceImpl& device, VkQueueFlags required, VkQueueFlags excluded)
{
    for (uint32_t i = 0; i < device.queueFamilyCount; ++i)
    {
        VkQueueFlags flags = device.queueFamilyFlags[i];
        if ((flags & required) == required && !(flags & excluded) && device.queueFamilyQueueCount[i] > 0)
        {
            return i;
        }
    }
    return noFamily;
}

// Dedicated families where they exist; otherwise queues share the graphics
// family, and beyond its queue count share hardware queues (ADR 0009).
void selectFamilies(const GraphicsDeviceImpl& device, uint32_t families[3])
{
    uint32_t graphics = findFamily(device, VK_QUEUE_GRAPHICS_BIT, 0);
    uint32_t compute = findFamily(device, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT);
    uint32_t copy = findFamily(device, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
    families[static_cast<size_t>(CommandQueueType::Graphics)] = graphics;
    families[static_cast<size_t>(CommandQueueType::Compute)] = compute != noFamily ? compute : graphics;
    families[static_cast<size_t>(CommandQueueType::Copy)] =
        copy != noFamily ? copy : families[static_cast<size_t>(CommandQueueType::Compute)];
}

bool hasExtension(const Vector<VkExtensionProperties>& extensions, const char* name)
{
    for (const VkExtensionProperties& extension : extensions)
    {
        if (std::strcmp(extension.extensionName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

Status createHeap(GraphicsDeviceImpl& device, uint32_t slots, VkDeviceSize descriptorSize,
                  VkDeviceSize descriptorAlignment, VkDeviceSize reservedRange, VkDeviceSize heapAlignment,
                  VkDeviceSize maxHeapSize, const char* name, DescriptorHeap& heap)
{
    heap.stride = alignUp(descriptorSize, descriptorAlignment);
    heap.slots = slots;
    heap.reservedOffset = alignUp(heap.stride * slots, heap.stride);
    heap.reservedSize = alignUp(reservedRange, heap.stride);
    VkDeviceSize heapSize = heap.reservedOffset + heap.reservedSize;
    if (heapSize > maxHeapSize)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::initialize: the %s heap (%llu bytes) exceeds the device limit", name,
                   static_cast<unsigned long long>(heapSize));
        return Status::Unsupported;
    }

    // Over-allocate so the heap's start can be aligned to the heap alignment.
    DedicatedBuffer buffer;
    Status status = createDedicatedBuffer(device, heapSize + heapAlignment, VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, buffer);
    if (status != Status::Ok)
    {
        return status;
    }
    VkDeviceSize offset = alignUp(buffer.address, heapAlignment) - buffer.address;
    heap.buffer = buffer.buffer;
    heap.memory = buffer.memory;
    heap.mapped = static_cast<uint8_t*>(buffer.mapped) + offset;
    heap.address = buffer.address + offset;
    setObjectName(device, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(heap.buffer), name);
    return Status::Ok;
}

void destroyHeap(GraphicsDeviceImpl& device, DescriptorHeap& heap)
{
    DedicatedBuffer buffer;
    buffer.buffer = heap.buffer;
    buffer.memory = heap.memory;
    buffer.mapped = heap.mapped;
    destroyDedicatedBuffer(device, buffer);
    heap = {};
}

void destroyQueues(GraphicsDeviceImpl& device)
{
    for (Vector<CommandQueue*>& queues : device.queues)
    {
        for (CommandQueue* queue : queues)
        {
            device.instance->allocator.destroy(static_cast<CommandQueueImpl*>(queue));
        }
        queues.clear();
    }
    for (std::mutex* lock : device.queueLocks)
    {
        device.instance->allocator.destroy(lock);
    }
    device.queueLocks.clear();
}

} // namespace

Status toStatus(GraphicsDeviceImpl& device, VkResult result)
{
    switch (result)
    {
    case VK_SUCCESS:
        return Status::Ok;
    case VK_TIMEOUT:
        return Status::Timeout;
    case VK_SUBOPTIMAL_KHR:
        return Status::Suboptimal;
    case VK_ERROR_OUT_OF_DATE_KHR:
        return Status::OutOfDate;
    case VK_ERROR_DEVICE_LOST:
        return reportDeviceLost(&device);
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return Status::OutOfHostMemory;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return Status::OutOfDeviceMemory;
    default:
        return Status::Unsupported;
    }
}

void setObjectName(GraphicsDeviceImpl& device, VkObjectType type, uint64_t handle, const char* name)
{
    InstanceImpl& instance = instanceOf(device);
    if (!name || !instance.debugUtils || !instance.fn.vkSetDebugUtilsObjectNameEXT)
    {
        return;
    }
    VkDebugUtilsObjectNameInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.objectType = type;
    info.objectHandle = handle;
    info.pObjectName = name;
    instance.fn.vkSetDebugUtilsObjectNameEXT(device.device, &info);
}

Result<void> initializeDevice(GraphicsDeviceCommon* common, const GraphicsDeviceInitDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    InstanceImpl& instance = instanceOf(device);

    // Queue families and indices (ADRs 0009, 0054).
    uint32_t families[3];
    selectFamilies(device, families);
    const uint32_t requested[3] = {desc.graphicsQueueCount, desc.computeQueueCount, desc.copyQueueCount};
    struct Assignment
    {
        uint32_t family;
        uint32_t index;
    };
    Vector<Assignment> assignments[3] = {Vector<Assignment>(&instance.allocator),
                                         Vector<Assignment>(&instance.allocator),
                                         Vector<Assignment>(&instance.allocator)};
    uint32_t used[maxQueueFamilies] = {};
    for (size_t type = 0; type < 3; ++type)
    {
        uint32_t family = families[type];
        for (uint32_t i = 0; i < requested[type]; ++i)
        {
            uint32_t available = device.queueFamilyQueueCount[family];
            // Beyond the family's queue count, wrap around and share.
            uint32_t index = used[family] < available ? used[family] : used[family] % available;
            ++used[family];
            if (!assignments[type].pushBack({family, index}))
            {
                return Status::OutOfHostMemory;
            }
        }
    }

    float priorities[64];
    for (float& priority : priorities)
    {
        priority = 1.0f;
    }
    VkDeviceQueueCreateInfo queueInfos[maxQueueFamilies] = {};
    uint32_t queueInfoCount = 0;
    for (uint32_t family = 0; family < device.queueFamilyCount; ++family)
    {
        if (used[family] == 0)
        {
            continue;
        }
        uint32_t count =
            used[family] < device.queueFamilyQueueCount[family] ? used[family] : device.queueFamilyQueueCount[family];
        count = count < 64 ? count : 64;
        VkDeviceQueueCreateInfo& info = queueInfos[queueInfoCount++];
        info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        info.queueFamilyIndex = family;
        info.queueCount = count;
        info.pQueuePriorities = priorities;
    }

    // Extensions: required, plus optional ones when present (ADRs 0013, 0019, 0056).
    uint32_t extensionCount = 0;
    instance.fn.vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &extensionCount, nullptr);
    Vector<VkExtensionProperties> available(&instance.allocator);
    if (!available.reserve(extensionCount))
    {
        return Status::OutOfHostMemory;
    }
    for (uint32_t i = 0; i < extensionCount; ++i)
    {
        (void)available.pushBack({});
    }
    instance.fn.vkEnumerateDeviceExtensionProperties(device.physicalDevice, nullptr, &extensionCount, available.data());
    Vector<const char*> extensions(&instance.allocator);
    const char* required[] = {VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME, VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME};
    // swapchain_maintenance1 only alongside the matching instance extension.
    const char* swapchainMaintenance = instance.surfaceMaintenanceKHR   ? VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME
                                       : instance.surfaceMaintenanceEXT ? VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME
                                                                        : nullptr;
    const char* optional[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
                              swapchainMaintenance};
    for (const char* name : required)
    {
        if (!extensions.pushBack(name))
        {
            return Status::OutOfHostMemory;
        }
    }
    for (const char* name : optional)
    {
        if (name && hasExtension(available, name) && !extensions.pushBack(name))
        {
            return Status::OutOfHostMemory;
        }
    }

    // Features: the baseline (ADR 0012).
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedPointers = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR};
    untypedPointers.shaderUntypedPointers = VK_TRUE;
    VkPhysicalDeviceDescriptorHeapFeaturesEXT heapFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT};
    heapFeatures.pNext = &untypedPointers;
    heapFeatures.descriptorHeap = VK_TRUE;
    VkPhysicalDeviceVulkan14Features features14 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES};
    features14.pNext = &heapFeatures;
    features14.maintenance5 = VK_TRUE;
    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.pNext = &features14;
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;
    features13.maintenance4 = VK_TRUE;
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.pNext = &features13;
    features12.timelineSemaphore = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;
    features12.drawIndirectCount = VK_TRUE;
    features12.samplerMirrorClampToEdge = VK_TRUE;
    features12.separateDepthStencilLayouts = VK_TRUE; // per-plane depth/stencil barriers (ADR 0049)
    VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &features12;
    VkPhysicalDeviceFeatures& core = features.features;
    core.multiViewport = VK_TRUE;
    core.fillModeNonSolid = VK_TRUE;
    core.depthBiasClamp = VK_TRUE;
    core.depthClamp = VK_TRUE;
    core.independentBlend = VK_TRUE;
    core.dualSrcBlend = VK_TRUE;
    core.multiDrawIndirect = VK_TRUE;
    core.shaderStorageImageExtendedFormats = VK_TRUE;
    core.textureCompressionBC = VK_TRUE;
    core.samplerAnisotropy = VK_TRUE;
    core.imageCubeArray = VK_TRUE;

    VkDeviceCreateInfo createInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    createInfo.pNext = &features;
    createInfo.queueCreateInfoCount = queueInfoCount;
    createInfo.pQueueCreateInfos = queueInfos;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    VkResult result = instance.fn.vkCreateDevice(device.physicalDevice, &createInfo, nullptr, &device.device);
    if (result != VK_SUCCESS)
    {
        logMessage(messageSink(&instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::initialize: vkCreateDevice failed (%d)", static_cast<int>(result));
        device.device = VK_NULL_HANDLE;
        return toStatus(device, result);
    }
    loadDeviceFunctions(instance.fn, device.device, device.fn);

    // Queues. A VkQueue used by several Slag queues gets one shared lock.
    for (size_t type = 0; type < 3; ++type)
    {
        for (const Assignment& assignment : assignments[type])
        {
            auto* queue = instance.allocator.create<CommandQueueImpl>();
            if (!queue || !device.queues[type].pushBack(queue))
            {
                instance.allocator.destroy(queue);
                shutdownDevice(&device);
                return Status::OutOfHostMemory;
            }
            queue->device = &device;
            queue->functions = device.functions;
            queue->type = static_cast<CommandQueueType>(type);
            queue->family = assignment.family;
            queue->waitInfos = Vector<VkSemaphoreSubmitInfo>(&instance.allocator);
            queue->signalInfos = Vector<VkSemaphoreSubmitInfo>(&instance.allocator);
            queue->commandBufferInfos = Vector<VkCommandBufferSubmitInfo>(&instance.allocator);
            device.fn.vkGetDeviceQueue(device.device, assignment.family, assignment.index, &queue->queue);
            bool known = false;
            for (uint32_t i = 0; i < device.sharingFamilyCount; ++i)
            {
                known = known || device.sharingFamilies[i] == assignment.family;
            }
            if (!known)
            {
                device.sharingFamilies[device.sharingFamilyCount++] = assignment.family;
            }
        }
    }
    for (size_t type = 0; type < 3; ++type)
    {
        for (CommandQueue* a : device.queues[type])
        {
            auto* queueA = static_cast<CommandQueueImpl*>(a);
            for (size_t otherType = 0; otherType < 3; ++otherType)
            {
                for (CommandQueue* b : device.queues[otherType])
                {
                    auto* queueB = static_cast<CommandQueueImpl*>(b);
                    if (queueA == queueB || queueA->queue != queueB->queue)
                    {
                        continue;
                    }
                    if (!queueA->sharedLock && !queueB->sharedLock)
                    {
                        auto* lock = instance.allocator.create<std::mutex>();
                        if (!lock || !device.queueLocks.pushBack(lock))
                        {
                            instance.allocator.destroy(lock);
                            shutdownDevice(&device);
                            return Status::OutOfHostMemory;
                        }
                        queueA->sharedLock = lock;
                    }
                    if (!queueA->sharedLock)
                    {
                        queueA->sharedLock = queueB->sharedLock;
                    }
                    queueB->sharedLock = queueA->sharedLock;
                }
            }
        }
    }

    Status blockStatus = selectBlockMemoryTypes(device);
    if (blockStatus != Status::Ok)
    {
        logMessage(messageSink(&instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::initialize: no memory type fits every resource type");
        shutdownDevice(&device);
        return blockStatus;
    }

    // Descriptor heaps, with the driver-reserved range after the user slots (ADRs 0029, 0032).
    const VkPhysicalDeviceDescriptorHeapPropertiesEXT& heap = device.heapProperties;
    VkDeviceSize resourceDescriptorSize =
        heap.imageDescriptorSize > heap.bufferDescriptorSize ? heap.imageDescriptorSize : heap.bufferDescriptorSize;
    VkDeviceSize resourceAlignment = heap.imageDescriptorAlignment > heap.bufferDescriptorAlignment
                                         ? heap.imageDescriptorAlignment
                                         : heap.bufferDescriptorAlignment;
    Status status = createHeap(device, desc.resourceHeapSize, resourceDescriptorSize, resourceAlignment,
                               heap.minResourceHeapReservedRange, heap.resourceHeapAlignment, heap.maxResourceHeapSize,
                               "slag resource heap", device.resourceHeap);
    if (status == Status::Ok)
    {
        status = createHeap(device, desc.samplerHeapSize, heap.samplerDescriptorSize, heap.samplerDescriptorAlignment,
                            heap.minSamplerHeapReservedRange, heap.samplerHeapAlignment, heap.maxSamplerHeapSize,
                            "slag sampler heap", device.samplerHeap);
    }
    if (status != Status::Ok)
    {
        shutdownDevice(&device);
        return status;
    }
    return {};
}

void shutdownDevice(GraphicsDeviceCommon* common)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    if (device.device == VK_NULL_HANDLE)
    {
        return;
    }
    device.fn.vkDeviceWaitIdle(device.device);
    destroyHeap(device, device.resourceHeap);
    destroyHeap(device, device.samplerHeap);
    destroyQueues(device);
    device.fn.vkDestroyDevice(device.device, nullptr);
    device.device = VK_NULL_HANDLE;
    device.fn = {};
    device.sharingFamilyCount = 0;
    device.initialized = false;
}

MemoryBudget memoryBudget(const GraphicsDeviceCommon* common)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    const InstanceImpl& instance = *static_cast<const InstanceImpl*>(device.instance);
    VkPhysicalDeviceMemoryBudgetPropertiesEXT budget = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
    VkPhysicalDeviceMemoryProperties2 properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
    if (device.memoryBudgetExtension)
    {
        properties.pNext = &budget;
    }
    instance.fn.vkGetPhysicalDeviceMemoryProperties2(device.physicalDevice, &properties);

    // Without VK_EXT_memory_budget, the heap sizes are the budget and usage is unknown (0).
    MemoryBudget result;
    const VkPhysicalDeviceMemoryProperties& memory = properties.memoryProperties;
    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
    {
        uint64_t heapBudget = device.memoryBudgetExtension ? budget.heapBudget[i] : memory.memoryHeaps[i].size;
        uint64_t heapUsage = device.memoryBudgetExtension ? budget.heapUsage[i] : 0;
        if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            result.dedicatedBudgetBytes += heapBudget;
            result.dedicatedUsageBytes += heapUsage;
        }
        else
        {
            result.sharedBudgetBytes += heapBudget;
            result.sharedUsageBytes += heapUsage;
        }
    }
    return result;
}

Result<void> deviceWaitIdle(GraphicsDeviceCommon* common)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    return toStatus(device, device.fn.vkDeviceWaitIdle(device.device));
}

Result<void> queueWaitIdle(CommandQueueCommon* common)
{
    auto* queue = static_cast<CommandQueueImpl*>(common);
    auto& device = *static_cast<GraphicsDeviceImpl*>(queue->device);
    VkResult result;
    if (queue->sharedLock)
    {
        std::lock_guard<std::mutex> lock(*queue->sharedLock);
        result = device.fn.vkQueueWaitIdle(queue->queue);
    }
    else
    {
        result = device.fn.vkQueueWaitIdle(queue->queue);
    }
    return toStatus(device, result);
}

} // namespace slag::vk
