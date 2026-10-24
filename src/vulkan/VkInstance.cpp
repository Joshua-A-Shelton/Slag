#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"

#include <slag/Limits.h>

#include <cstdio>
#include <cstring>

namespace slag::vk {

namespace {

constexpr const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

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

bool instanceExtensions(const GlobalFunctions& global, const char* layer, Vector<VkExtensionProperties>& extensions)
{
    uint32_t count = 0;
    if (global.vkEnumerateInstanceExtensionProperties(layer, &count, nullptr) != VK_SUCCESS)
    {
        return true; // e.g. the layer isn't installed: no extensions
    }
    if (!extensions.reserve(count))
    {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        (void)extensions.pushBack({}); // capacity reserved above
    }
    global.vkEnumerateInstanceExtensionProperties(layer, &count, extensions.data());
    while (extensions.size() > count)
    {
        extensions.popBack();
    }
    return true;
}

bool hasLayer(const GlobalFunctions& global, const HostAllocator& allocator, const char* name)
{
    uint32_t count = 0;
    global.vkEnumerateInstanceLayerProperties(&count, nullptr);
    Vector<VkLayerProperties> layers(&allocator);
    if (!layers.reserve(count))
    {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        (void)layers.pushBack({});
    }
    global.vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (uint32_t i = 0; i < count; ++i)
    {
        if (std::strcmp(layers[i].layerName, name) == 0)
        {
            return true;
        }
    }
    return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                              VkDebugUtilsMessageTypeFlagsEXT,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData)
{
    MessageSeverity mapped = MessageSeverity::Info;
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        mapped = MessageSeverity::Error;
    }
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        mapped = MessageSeverity::Warning;
    }
    logMessage(messageSink(static_cast<const InstanceCommon*>(userData)), mapped, MessageSource::Backend, "%s",
               data && data->pMessage ? data->pMessage : "");
    return VK_FALSE; // Slag never aborts the call (ADR 0011)
}

VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo(InstanceImpl* instance)
{
    VkDebugUtilsMessengerCreateInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debugMessenger;
    info.pUserData = static_cast<InstanceCommon*>(instance);
    return info;
}

// Everything the baseline check (ADR 0012) and device info need.
struct DeviceQuery
{
    VkPhysicalDeviceProperties2 properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceVulkan11Properties properties11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
    VkPhysicalDeviceVulkan12Properties properties12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
    VkPhysicalDeviceVulkan13Properties properties13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT};

    VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceDescriptorHeapFeaturesEXT heapFeatures = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT};
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedPointers = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR};

    VkPhysicalDeviceMemoryProperties2 memory = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};

    bool descriptorHeapExtension = false;
    bool untypedPointersExtension = false;
    bool memoryBudgetExtension = false;
    bool conservativeRasterizationExtension = false;
    // Presentation (ADRs 0013, 0056, 0057, 0059).
    bool swapchainExtension = false;
    bool swapchainMaintenanceExtension = false; // the one matching the instance's surface_maintenance1
    bool mutableFormatExtension = false;
    bool presentIdExtension = false;
    bool presentWaitExtension = false;
    bool hdrMetadataExtension = false;
    bool calibratedTimestampsExtension = false;
    VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR swapchainMaintenance = {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR};
    VkPhysicalDevicePresentIdFeaturesKHR presentId = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR};
    VkPhysicalDevicePresentWaitFeaturesKHR presentWait = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR};
    bool graphicsQueue = false;
    uint32_t queueFamilyCount = 0;
    VkQueueFlags queueFamilyFlags[maxQueueFamilies] = {};
    uint32_t queueFamilyQueueCount[maxQueueFamilies] = {};
    uint32_t queueFamilyTimestampBits[maxQueueFamilies] = {};
};

bool queryDevice(const InstanceImpl& instance, VkPhysicalDevice device, DeviceQuery& query)
{
    uint32_t count = 0;
    instance.fn.vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    Vector<VkExtensionProperties> extensions(&instance.allocator);
    if (!extensions.reserve(count))
    {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        (void)extensions.pushBack({});
    }
    instance.fn.vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data());
    query.descriptorHeapExtension = hasExtension(extensions, VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME);
    query.untypedPointersExtension = hasExtension(extensions, VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME);
    query.memoryBudgetExtension = hasExtension(extensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    query.conservativeRasterizationExtension =
        hasExtension(extensions, VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
    query.swapchainExtension = hasExtension(extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    query.swapchainMaintenanceExtension =
        (instance.surfaceMaintenanceKHR && hasExtension(extensions, VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) ||
        (instance.surfaceMaintenanceEXT && hasExtension(extensions, VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME));
    query.mutableFormatExtension = hasExtension(extensions, VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
    query.presentIdExtension = hasExtension(extensions, VK_KHR_PRESENT_ID_EXTENSION_NAME);
    query.presentWaitExtension = hasExtension(extensions, VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
    query.hdrMetadataExtension = hasExtension(extensions, VK_EXT_HDR_METADATA_EXTENSION_NAME);
    query.calibratedTimestampsExtension = hasExtension(extensions, VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);

    // Extension structs may only be chained when the extension exists.
    query.properties.pNext = &query.properties11;
    query.properties11.pNext = &query.properties12;
    query.properties12.pNext = &query.properties13;
    query.features.pNext = &query.features11;
    query.features11.pNext = &query.features12;
    query.features12.pNext = &query.features13;
    void** propertiesTail = &query.properties13.pNext;
    void** featuresTail = &query.features13.pNext;
    if (query.descriptorHeapExtension)
    {
        *propertiesTail = &query.heapProperties;
        propertiesTail = &query.heapProperties.pNext;
        *featuresTail = &query.heapFeatures;
        featuresTail = &query.heapFeatures.pNext;
    }
    if (query.untypedPointersExtension)
    {
        *featuresTail = &query.untypedPointers;
        featuresTail = &query.untypedPointers.pNext;
    }
    if (query.swapchainMaintenanceExtension)
    {
        *featuresTail = &query.swapchainMaintenance;
        featuresTail = &query.swapchainMaintenance.pNext;
    }
    if (query.presentIdExtension)
    {
        *featuresTail = &query.presentId;
        featuresTail = &query.presentId.pNext;
    }
    if (query.presentWaitExtension)
    {
        *featuresTail = &query.presentWait;
        featuresTail = &query.presentWait.pNext;
    }
    instance.fn.vkGetPhysicalDeviceProperties2(device, &query.properties);
    instance.fn.vkGetPhysicalDeviceFeatures2(device, &query.features);
    instance.fn.vkGetPhysicalDeviceMemoryProperties2(device, &query.memory);

    uint32_t familyCount = 0;
    instance.fn.vkGetPhysicalDeviceQueueFamilyProperties2(device, &familyCount, nullptr);
    Vector<VkQueueFamilyProperties2> families(&instance.allocator);
    if (!families.reserve(familyCount))
    {
        return false;
    }
    for (uint32_t i = 0; i < familyCount; ++i)
    {
        (void)families.pushBack({VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    }
    instance.fn.vkGetPhysicalDeviceQueueFamilyProperties2(device, &familyCount, families.data());
    query.queueFamilyCount = familyCount < maxQueueFamilies ? familyCount : maxQueueFamilies;
    for (uint32_t i = 0; i < query.queueFamilyCount; ++i)
    {
        query.queueFamilyFlags[i] = families[i].queueFamilyProperties.queueFlags;
        query.queueFamilyQueueCount[i] = families[i].queueFamilyProperties.queueCount;
        query.queueFamilyTimestampBits[i] = families[i].queueFamilyProperties.timestampValidBits;
        if (query.queueFamilyFlags[i] & VK_QUEUE_GRAPHICS_BIT)
        {
            query.graphicsQueue = true;
        }
    }
    return true;
}

// Returns why a device doesn't meet the baseline (ADRs 0012, 0028), or null.
const char* baselineFailure(const DeviceQuery& query, bool allowSoftwareDevices)
{
    const VkPhysicalDeviceProperties& properties = query.properties.properties;
    const VkPhysicalDeviceLimits& deviceLimits = properties.limits;
    const VkPhysicalDeviceFeatures& features = query.features.features;

    if (properties.apiVersion < VK_API_VERSION_1_4)
    {
        return "Vulkan 1.4 is not supported";
    }
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU && !allowSoftwareDevices)
    {
        return "software device (allowSoftwareDevices is off)";
    }
    if (!query.descriptorHeapExtension || !query.heapFeatures.descriptorHeap)
    {
        return "VK_EXT_descriptor_heap is not supported";
    }
    if (!query.untypedPointersExtension || !query.untypedPointers.shaderUntypedPointers)
    {
        return "VK_KHR_shader_untyped_pointers is not supported";
    }
    if (!query.graphicsQueue)
    {
        return "no graphics queue family";
    }

    struct FeatureCheck
    {
        VkBool32 supported;
        const char* name;
    };
    const FeatureCheck featureChecks[] = {
        {query.features13.dynamicRendering, "dynamicRendering"},
        {query.features13.synchronization2, "synchronization2"},
        {query.features12.timelineSemaphore, "timelineSemaphore"},
        {query.features12.hostQueryReset, "hostQueryReset"},
        {deviceLimits.timestampComputeAndGraphics, "timestampComputeAndGraphics"}, // ADR 0051
        {query.features11.shaderDrawParameters, "shaderDrawParameters"}, // SV_StartInstanceLocation (ADR 0047)
        {query.features12.bufferDeviceAddress, "bufferDeviceAddress"},
        {query.features12.drawIndirectCount, "drawIndirectCount"},
        {query.features12.samplerMirrorClampToEdge, "samplerMirrorClampToEdge"},
        {query.features12.separateDepthStencilLayouts, "separateDepthStencilLayouts"},
        {features.multiViewport, "multiViewport"},
        {features.fillModeNonSolid, "fillModeNonSolid"},
        {features.depthBiasClamp, "depthBiasClamp"},
        {features.depthClamp, "depthClamp"},
        {features.independentBlend, "independentBlend"},
        {features.dualSrcBlend, "dualSrcBlend"},
        {features.multiDrawIndirect, "multiDrawIndirect"},
        {features.shaderStorageImageExtendedFormats, "shaderStorageImageExtendedFormats"},
        {features.textureCompressionBC, "textureCompressionBC"},
        {features.samplerAnisotropy, "samplerAnisotropy"},
        {features.imageCubeArray, "imageCubeArray"},
    };
    for (const FeatureCheck& check : featureChecks)
    {
        if (!check.supported)
        {
            return check.name;
        }
    }

    struct LimitCheck
    {
        bool met;
        const char* name;
    };
    const LimitCheck limitChecks[] = {
        {deviceLimits.maxImageDimension1D >= limits::maxTexture1DSize, "maxImageDimension1D"},
        {deviceLimits.maxImageDimension2D >= limits::maxTexture2DSize, "maxImageDimension2D"},
        {deviceLimits.maxImageDimension3D >= limits::maxTexture3DSize, "maxImageDimension3D"},
        {deviceLimits.maxImageDimensionCube >= limits::maxTextureCubeSize, "maxImageDimensionCube"},
        {deviceLimits.maxImageArrayLayers >= limits::maxTextureArrayLayers, "maxImageArrayLayers"},
        {deviceLimits.maxTexelBufferElements >= limits::maxTypedBufferElements, "maxTexelBufferElements"},
        {deviceLimits.maxStorageBufferRange >= limits::maxStorageBufferViewSize, "maxStorageBufferRange"},
        {deviceLimits.maxUniformBufferRange >= limits::maxConstantBufferSize, "maxUniformBufferRange"},
        {deviceLimits.maxColorAttachments >= limits::maxColorTargets, "maxColorAttachments"},
        {deviceLimits.maxViewports >= limits::maxViewports, "maxViewports"},
        {deviceLimits.maxVertexInputAttributes >= limits::maxVertexAttributes, "maxVertexInputAttributes"},
        {deviceLimits.maxVertexInputBindings >= limits::maxVertexBufferSlots, "maxVertexInputBindings"},
        {deviceLimits.maxSamplerAnisotropy >= static_cast<float>(limits::maxAnisotropy), "maxSamplerAnisotropy"},
        {(deviceLimits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT) != 0, "4x color MSAA"},
        {(deviceLimits.framebufferDepthSampleCounts & VK_SAMPLE_COUNT_4_BIT) != 0, "4x depth MSAA"},
        // 128 bytes of push constants plus the constant buffer slot's address (ADRs 0003, 0044).
        {query.heapProperties.maxPushDataSize >= limits::maxPushConstantsSize + 8, "maxPushDataSize"},
    };
    for (const LimitCheck& check : limitChecks)
    {
        if (!check.met)
        {
            return check.name;
        }
    }
    return nullptr;
}

GraphicsDeviceType deviceType(VkPhysicalDeviceType type)
{
    switch (type)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return GraphicsDeviceType::Integrated;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return GraphicsDeviceType::Software;
    default:
        // Virtual and "other" GPUs are treated as discrete (ADR 0008).
        return GraphicsDeviceType::Discrete;
    }
}

void fillInfo(const DeviceQuery& query, GraphicsDeviceInfo& info)
{
    const VkPhysicalDeviceProperties& properties = query.properties.properties;
    std::snprintf(info.name, sizeof(info.name), "%s", properties.deviceName);
    if (query.properties12.driverInfo[0] != '\0')
    {
        std::snprintf(info.driverVersion, sizeof(info.driverVersion), "%.63s", query.properties12.driverInfo);
    }
    else
    {
        std::snprintf(info.driverVersion, sizeof(info.driverVersion), "%u", properties.driverVersion);
    }
    info.vendorId = properties.vendorID;
    info.deviceId = properties.deviceID;
    info.deviceType = deviceType(properties.deviceType);
    const VkPhysicalDeviceMemoryProperties& memory = query.memory.memoryProperties;
    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
    {
        if (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            info.dedicatedMemoryBytes += memory.memoryHeaps[i].size;
        }
        else
        {
            info.sharedMemoryBytes += memory.memoryHeaps[i].size;
        }
    }
    if (query.properties11.deviceLUIDValid)
    {
        std::memcpy(info.luid, query.properties11.deviceLUID, sizeof(info.luid));
    }
}

// ADR 0019. GpuUpload needs full resizable BAR: a CPU-visible device-local
// heap about the size of VRAM, not the 256 MB window.
void fillCapabilities(const DeviceQuery& query, const GraphicsDeviceInfo& info,
                      GraphicsDeviceCapabilities& capabilities)
{
    // Integrated GPUs, matching D3D12's UMA; RADV still lists their system
    // memory as a separate, non-device-local heap. Otherwise, every heap is
    // device-local.
    const VkPhysicalDeviceMemoryProperties& memory = query.memory.memoryProperties;
    bool allDeviceLocal = true;
    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
    {
        allDeviceLocal = allDeviceLocal && (memory.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0;
    }
    capabilities.unifiedMemory = info.deviceType == GraphicsDeviceType::Integrated || allDeviceLocal;
    constexpr VkMemoryPropertyFlags gpuUpload = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    for (uint32_t i = 0; i < memory.memoryTypeCount; ++i)
    {
        VkDeviceSize heapSize = memory.memoryHeaps[memory.memoryTypes[i].heapIndex].size;
        if ((memory.memoryTypes[i].propertyFlags & gpuUpload) == gpuUpload &&
            heapSize * 10 >= info.dedicatedMemoryBytes * 9)
        {
            capabilities.gpuUploadMemory = true;
        }
    }
    capabilities.maxBufferSize = query.properties13.maxBufferSize;
    capabilities.samplerReduction = query.features12.samplerFilterMinmax == VK_TRUE;
    capabilities.conservativeRasterization = query.conservativeRasterizationExtension; // overestimate is the base mode
    capabilities.depthBounds = query.features.features.depthBounds == VK_TRUE;
    capabilities.frameLatency = query.presentIdExtension && query.presentWaitExtension && query.presentId.presentId &&
                                query.presentWait.presentWait;
}

// ADR 0051: valid bits of the families the queues use (timestampComputeAndGraphics
// covers graphics and compute); calibration needs the device and host clocks.
void fillTimestampCapabilities(const InstanceImpl& instance, const DeviceQuery& query, GraphicsDeviceImpl& device)
{
    uint32_t families[3] = {};
    selectFamilies(device, families);
    GraphicsDeviceCapabilities& capabilities = device.capabilities;
    uint32_t bits = 64;
    for (CommandQueueType type : {CommandQueueType::Graphics, CommandQueueType::Compute})
    {
        uint32_t familyBits = query.queueFamilyTimestampBits[families[static_cast<size_t>(type)]];
        bits = familyBits < bits ? familyBits : bits;
    }
    // Copying and resetting queries needs a graphics or compute family, so a
    // dedicated transfer family (NVIDIA's) can't resolve them.
    uint32_t copyFamily = families[static_cast<size_t>(CommandQueueType::Copy)];
    uint32_t copyBits = query.queueFamilyTimestampBits[copyFamily];
    capabilities.copyQueueTimestamps =
        copyBits > 0 && (query.queueFamilyFlags[copyFamily] & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) != 0;
    if (capabilities.copyQueueTimestamps && copyBits < bits)
    {
        bits = copyBits;
    }
    capabilities.timestampValidBits = bits;

    if (query.calibratedTimestampsExtension && instance.fn.vkGetPhysicalDeviceCalibrateableTimeDomainsKHR)
    {
        VkTimeDomainKHR domains[8] = {};
        uint32_t count = 8;
        VkResult result =
            instance.fn.vkGetPhysicalDeviceCalibrateableTimeDomainsKHR(device.physicalDevice, &count, domains);
        bool deviceDomain = false;
        bool hostDomain = false;
        for (uint32_t i = 0; (result == VK_SUCCESS || result == VK_INCOMPLETE) && i < count; ++i)
        {
            deviceDomain = deviceDomain || domains[i] == VK_TIME_DOMAIN_DEVICE_KHR;
            hostDomain = hostDomain || domains[i] == hostTimeDomain;
        }
        device.calibratedTimestamps = deviceDomain && hostDomain;
    }
    capabilities.timestampCalibration = device.calibratedTimestamps;
}

// Lower sorts first (ADR 0008).
uint32_t typeRank(GraphicsDeviceType type, DevicePreference preference)
{
    switch (type)
    {
    case GraphicsDeviceType::Discrete:
        return preference == DevicePreference::HighPerformance ? 0 : 1;
    case GraphicsDeviceType::Integrated:
        return preference == DevicePreference::HighPerformance ? 1 : 0;
    case GraphicsDeviceType::Software:
        return 2;
    }
    return 3;
}

bool sortsBefore(const GraphicsDeviceImpl& a, const GraphicsDeviceImpl& b, DevicePreference preference)
{
    uint32_t rankA = typeRank(a.info.deviceType, preference);
    uint32_t rankB = typeRank(b.info.deviceType, preference);
    if (rankA != rankB)
    {
        return rankA < rankB;
    }
    return a.info.dedicatedMemoryBytes > b.info.dedicatedMemoryBytes;
}

Status listDevices(InstanceImpl& instance, const InstanceDesc& desc)
{
    uint32_t count = 0;
    if (instance.fn.vkEnumeratePhysicalDevices(instance.instance, &count, nullptr) != VK_SUCCESS)
    {
        return Status::Ok; // no devices is not an error (ADR 0012)
    }
    Vector<VkPhysicalDevice> physicalDevices(&instance.allocator);
    if (!physicalDevices.reserve(count))
    {
        return Status::OutOfHostMemory;
    }
    for (uint32_t i = 0; i < count; ++i)
    {
        (void)physicalDevices.pushBack(VK_NULL_HANDLE);
    }
    instance.fn.vkEnumeratePhysicalDevices(instance.instance, &count, physicalDevices.data());

    for (uint32_t i = 0; i < count; ++i)
    {
        DeviceQuery query;
        if (!queryDevice(instance, physicalDevices[i], query))
        {
            return Status::OutOfHostMemory;
        }
        const char* failure = baselineFailure(query, desc.allowSoftwareDevices);
        if (failure)
        {
            logMessage(messageSink(&instance), MessageSeverity::Info, MessageSource::Slag,
                       "slag: device \"%s\" skipped: %s", query.properties.properties.deviceName, failure);
            continue;
        }
        FormatSupport formatSupport[pixelFormatCount];
        queryFormatSupport(instance, physicalDevices[i], formatSupport);
        if (!meetsFormatBaseline(instance, query.properties.properties.deviceName, formatSupport))
        {
            continue;
        }

        auto* device = instance.allocator.create<GraphicsDeviceImpl>();
        if (!device)
        {
            return Status::OutOfHostMemory;
        }
        device->instance = &instance;
        device->functions = instance.functions;
        device->physicalDevice = physicalDevices[i];
        device->properties = query.properties.properties;
        device->heapProperties = query.heapProperties;
        device->heapProperties.pNext = nullptr;
        device->memoryProperties = query.memory.memoryProperties;
        device->queueFamilyCount = query.queueFamilyCount;
        for (uint32_t family = 0; family < query.queueFamilyCount; ++family)
        {
            device->queueFamilyFlags[family] = query.queueFamilyFlags[family];
            device->queueFamilyQueueCount[family] = query.queueFamilyQueueCount[family];
        }
        for (Vector<CommandQueue*>& queues : device->queues)
        {
            queues = Vector<CommandQueue*>(&instance.allocator);
        }
        device->queueLocks = Vector<std::mutex*>(&instance.allocator);
        fillInfo(query, device->info);
        fillCapabilities(query, device->info, device->capabilities);
        device->memoryBudgetExtension = query.memoryBudgetExtension;
        // Swapchains need swapchain_maintenance1 and mutable formats (ADR 0056).
        device->presentation = query.swapchainExtension && query.swapchainMaintenanceExtension &&
                               query.swapchainMaintenance.swapchainMaintenance1 && query.mutableFormatExtension;
        device->capabilities.frameLatency = device->capabilities.frameLatency && device->presentation;
        device->hdrMetadata = query.hdrMetadataExtension;
        fillTimestampCapabilities(instance, query, *device);
        for (size_t format = 0; format < pixelFormatCount; ++format)
        {
            device->formatSupport[format] = formatSupport[format];
        }

        // Insert in preference order.
        if (!instance.devices.pushBack(device))
        {
            instance.allocator.destroy(device);
            return Status::OutOfHostMemory;
        }
        for (size_t j = instance.devices.size() - 1; j > 0; --j)
        {
            auto* current = static_cast<GraphicsDeviceImpl*>(instance.devices[j]);
            auto* previous = static_cast<GraphicsDeviceImpl*>(instance.devices[j - 1]);
            if (!sortsBefore(*current, *previous, desc.devicePreference))
            {
                break;
            }
            instance.devices[j] = previous;
            instance.devices[j - 1] = current;
        }
    }
    return Status::Ok;
}

void destroyImpl(InstanceImpl* instance)
{
    for (GraphicsDevice* device : instance->devices)
    {
        auto* impl = static_cast<GraphicsDeviceImpl*>(device);
        if (impl->initialized)
        {
            shutdownDevice(impl); // the user should have shut it down; don't leak the VkDevice
        }
        instance->allocator.destroy(impl);
    }
    instance->devices.clear();
    if (instance->messenger != VK_NULL_HANDLE)
    {
        instance->fn.vkDestroyDebugUtilsMessengerEXT(instance->instance, instance->messenger, nullptr);
    }
    if (instance->instance != VK_NULL_HANDLE)
    {
        instance->fn.vkDestroyInstance(instance->instance, nullptr);
    }
    HostAllocator allocator = instance->allocator;
    allocator.destroy(instance);
}

} // namespace

Result<InstanceCommon*> createInstance(const InstanceDesc& desc, const HostAllocator& allocator)
{
    MessageSink sink = messageSink(desc);
    const GlobalFunctions* global = loadGlobalFunctions();
    if (!global)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: createInstance: the Vulkan loader library is not installed");
        return Status::Unsupported;
    }

    uint32_t loaderVersion = VK_API_VERSION_1_0;
    if (global->vkEnumerateInstanceVersion)
    {
        global->vkEnumerateInstanceVersion(&loaderVersion);
    }
    if (loaderVersion < VK_API_VERSION_1_4)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: createInstance: the Vulkan loader doesn't support Vulkan 1.4");
        return Status::Unsupported;
    }

    auto* instance = allocator.create<InstanceImpl>();
    if (!instance)
    {
        return Status::OutOfHostMemory;
    }
    initInstanceCommon(*instance, desc, *slagBackend_vulkan(), allocator);
    instance->global = global;

    Vector<VkExtensionProperties> available(&instance->allocator);
    if (!instanceExtensions(*global, nullptr, available))
    {
        destroyImpl(instance);
        return Status::OutOfHostMemory;
    }

    // Extensions: debug utils, and every surface extension that exists (ADRs 0013, 0056, 0059).
    const char* wanted[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
        "VK_KHR_xlib_surface",
        "VK_KHR_xcb_surface",
        "VK_KHR_wayland_surface",
        "VK_KHR_get_surface_capabilities2",
        "VK_EXT_swapchain_colorspace",
    };
    Vector<const char*> extensions(&instance->allocator);
    for (const char* name : wanted)
    {
        if (hasExtension(available, name) && !extensions.pushBack(name))
        {
            destroyImpl(instance);
            return Status::OutOfHostMemory;
        }
    }
    // Prefer the KHR surface_maintenance1; fall back to the EXT one.
    if (hasExtension(available, "VK_KHR_surface_maintenance1"))
    {
        instance->surfaceMaintenanceKHR = extensions.pushBack("VK_KHR_surface_maintenance1");
    }
    else if (hasExtension(available, "VK_EXT_surface_maintenance1"))
    {
        instance->surfaceMaintenanceEXT = extensions.pushBack("VK_EXT_surface_maintenance1");
    }
    instance->debugUtils = hasExtension(available, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    // Backend validation layers (ADR 0010); best effort, reported when missing.
    Vector<const char*> layers(&instance->allocator);
    VkBool32 settingTrue = VK_TRUE;
    VkLayerSettingEXT settings[2] = {};
    uint32_t settingCount = 0;
    VkLayerSettingsCreateInfoEXT layerSettings = {VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT};
    bool useLayerSettings = false;
    if (desc.validationFlags != ValidationFlags::None)
    {
        if (hasLayer(*global, instance->allocator, validationLayerName))
        {
            (void)layers.pushBack(validationLayerName);
            Vector<VkExtensionProperties> layerExtensions(&instance->allocator);
            (void)instanceExtensions(*global, validationLayerName, layerExtensions);
            bool settingsAvailable = hasExtension(layerExtensions, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
            if ((desc.validationFlags & ValidationFlags::Synchronization) != ValidationFlags::None)
            {
                settings[settingCount++] = {validationLayerName, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
                                            &settingTrue};
            }
            if ((desc.validationFlags & ValidationFlags::GpuAssisted) != ValidationFlags::None)
            {
                settings[settingCount++] = {validationLayerName, "gpuav_enable", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1,
                                            &settingTrue};
            }
            if (settingCount > 0)
            {
                if (settingsAvailable)
                {
                    useLayerSettings = extensions.pushBack(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
                }
                else
                {
                    logMessage(sink, MessageSeverity::Warning, MessageSource::Slag,
                               "slag: createInstance: the validation layer can't be configured; GPU-assisted and "
                               "synchronization validation are off");
                }
            }
        }
        else
        {
            logMessage(sink, MessageSeverity::Warning, MessageSource::Slag,
                       "slag: createInstance: %s is not installed; Vulkan validation is off", validationLayerName);
        }
    }

    VkApplicationInfo application = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName = desc.applicationName;
    application.applicationVersion = desc.applicationVersion;
    application.pEngineName = desc.engineName;
    application.engineVersion = desc.engineVersion;
    application.apiVersion = VK_API_VERSION_1_4;

    // Chaining the messenger reports problems during vkCreateInstance too (ADR 0011).
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = messengerCreateInfo(instance);
    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    const void** tail = &createInfo.pNext;
    if (instance->debugUtils)
    {
        *tail = &messengerInfo;
        tail = &messengerInfo.pNext;
    }
    if (useLayerSettings)
    {
        layerSettings.settingCount = settingCount;
        layerSettings.pSettings = settings;
        *tail = &layerSettings;
    }
    createInfo.pApplicationInfo = &application;
    createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createInfo.ppEnabledLayerNames = layers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = global->vkCreateInstance(&createInfo, nullptr, &instance->instance);
    if (result != VK_SUCCESS)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: createInstance: vkCreateInstance failed (%d)", static_cast<int>(result));
        instance->instance = VK_NULL_HANDLE;
        destroyImpl(instance);
        return result == VK_ERROR_OUT_OF_HOST_MEMORY ? Status::OutOfHostMemory : Status::Unsupported;
    }
    loadInstanceFunctions(*global, instance->instance, instance->fn);

    if (instance->debugUtils && instance->fn.vkCreateDebugUtilsMessengerEXT)
    {
        messengerInfo.pNext = nullptr;
        instance->fn.vkCreateDebugUtilsMessengerEXT(instance->instance, &messengerInfo, nullptr, &instance->messenger);
    }

    Status status = listDevices(*instance, desc);
    if (status != Status::Ok)
    {
        destroyImpl(instance);
        return status;
    }
    return static_cast<InstanceCommon*>(instance);
}

void destroyInstance(InstanceCommon* instance)
{
    destroyImpl(static_cast<InstanceImpl*>(instance));
}

} // namespace slag::vk
