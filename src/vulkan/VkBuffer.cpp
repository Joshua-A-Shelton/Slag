#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkMemory.h"

namespace slag::vk {

namespace {

// D3D12's buffer placement alignment, on both backends (ADR 0023).
constexpr VkDeviceSize bufferPlacementAlignment = 65536;

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

Result<BufferCommon*> createPlacedBuffer(GraphicsDeviceImpl& device, const BufferDesc& desc, BufferImpl* buffer)
{
    auto* block = static_cast<MemoryBlockImpl*>(desc.placement.memoryBlock);
    VkBufferCreateInfo info = bufferCreateInfo(device, desc.size, allBufferUsage);
    VkResult result = device.fn.vkCreateBuffer(device.device, &info, nullptr, &buffer->buffer);
    if (result == VK_SUCCESS)
    {
        result = device.fn.vkBindBufferMemory(device.device, buffer->buffer, block->memory, desc.placement.offset);
    }
    if (result != VK_SUCCESS)
    {
        if (buffer->buffer != VK_NULL_HANDLE)
        {
            device.fn.vkDestroyBuffer(device.device, buffer->buffer, nullptr);
        }
        device.instance->allocator.destroy(buffer);
        return toStatus(device, result);
    }
    VkBufferDeviceAddressInfo addressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    addressInfo.buffer = buffer->buffer;
    buffer->gpuAddress = device.fn.vkGetBufferDeviceAddress(device.device, &addressInfo);
    buffer->kind = block->kind;
    // A placed buffer's pointer is its block's mapping plus the offset (ADR 0024).
    buffer->mapped = block->mapped ? block->mapped + desc.placement.offset : nullptr;
    return static_cast<BufferCommon*>(buffer);
}

Result<BufferCommon*> createDedicatedBufferImpl(GraphicsDeviceImpl& device, const BufferDesc& desc, BufferImpl* buffer)
{
    if (desc.placement.kind == MemoryKind::GpuUpload && !device.capabilities.gpuUploadMemory)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createBuffer: no suitable memory for this memory kind on \"%s\"",
                   device.info.name);
        device.instance->allocator.destroy(buffer);
        return Status::Unsupported;
    }
    MemoryFlags flags = memoryFlags(desc.placement.kind);
    DedicatedBuffer allocation;
    Status status = createDedicatedBuffer(device, desc.size, allBufferUsage, flags.required, flags.preferred,
                                          flags.avoided, allocation);
    if (status == Status::Ok && desc.placement.kind == MemoryKind::GpuUpload &&
        !supportsGpuUpload(device, allocation.memoryType))
    {
        destroyDedicatedBuffer(device, allocation);
        status = Status::Unsupported;
    }
    if (status != Status::Ok)
    {
        device.instance->allocator.destroy(buffer);
        return status;
    }
    buffer->kind = desc.placement.kind;
    buffer->buffer = allocation.buffer;
    buffer->memory = allocation.memory;
    buffer->mapped = desc.placement.kind == MemoryKind::GpuOnly ? nullptr : allocation.mapped;
    buffer->gpuAddress = allocation.address;
    return static_cast<BufferCommon*>(buffer);
}

} // namespace

Result<MemoryBlockCommon*> createMemoryBlock(GraphicsDeviceCommon* common, const MemoryBlockDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    uint32_t memoryType = device.blockMemoryTypes[static_cast<size_t>(desc.kind)];
    if (memoryType == noMemoryType)
    {
        return Status::Unsupported;
    }
    auto* block = device.instance->allocator.create<MemoryBlockImpl>();
    if (!block)
    {
        return Status::OutOfHostMemory;
    }
    block->device = &device;
    block->functions = device.functions;
    block->size = desc.size;
    block->kind = desc.kind;

    VkMemoryAllocateFlagsInfo flags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocateInfo.pNext = &flags;
    allocateInfo.allocationSize = desc.size;
    allocateInfo.memoryTypeIndex = memoryType;
    VkResult result = device.fn.vkAllocateMemory(device.device, &allocateInfo, nullptr, &block->memory);
    if (result == VK_SUCCESS && desc.kind != MemoryKind::GpuOnly)
    {
        void* mapped = nullptr;
        result = device.fn.vkMapMemory(device.device, block->memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        block->mapped = static_cast<uint8_t*>(mapped);
    }
    if (result != VK_SUCCESS)
    {
        destroyMemoryBlock(&device, block);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(block->memory), desc.debugName);
    return static_cast<MemoryBlockCommon*>(block);
}

void destroyMemoryBlock(GraphicsDeviceCommon* common, MemoryBlockCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* block = static_cast<MemoryBlockImpl*>(object);
    if (block->memory != VK_NULL_HANDLE)
    {
        device.fn.vkFreeMemory(device.device, block->memory, nullptr); // also unmaps
    }
    device.instance->allocator.destroy(block);
}

Result<BufferCommon*> createBuffer(GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* buffer = device.instance->allocator.create<BufferImpl>();
    if (!buffer)
    {
        return Status::OutOfHostMemory;
    }
    buffer->device = &device;
    buffer->functions = device.functions;
    buffer->size = desc.size;
    Result<BufferCommon*> result = desc.placement.memoryBlock ? createPlacedBuffer(device, desc, buffer)
                                                              : createDedicatedBufferImpl(device, desc, buffer);
    if (result)
    {
        setObjectName(device, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer->buffer), desc.debugName);
    }
    return result;
}

void destroyBuffer(GraphicsDeviceCommon* common, BufferCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* buffer = static_cast<BufferImpl*>(object);
    if (buffer->memory == VK_NULL_HANDLE)
    {
        device.fn.vkDestroyBuffer(device.device, buffer->buffer, nullptr); // placed: the block keeps its memory
    }
    else
    {
        DedicatedBuffer allocation;
        allocation.buffer = buffer->buffer;
        allocation.memory = buffer->memory;
        allocation.mapped = buffer->mapped;
        destroyDedicatedBuffer(device, allocation);
    }
    device.instance->allocator.destroy(buffer);
}

PlacementInfo bufferPlacementInfo(const GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    VkBufferCreateInfo info = bufferCreateInfo(device, desc.size, allBufferUsage);
    VkDeviceBufferMemoryRequirements query = {VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS};
    query.pCreateInfo = &info;
    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    device.fn.vkGetDeviceBufferMemoryRequirements(device.device, &query, &requirements);
    PlacementInfo placement;
    placement.alignment = requirements.memoryRequirements.alignment > bufferPlacementAlignment
                              ? requirements.memoryRequirements.alignment
                              : bufferPlacementAlignment;
    placement.size = alignUp(requirements.memoryRequirements.size, placement.alignment);
    return placement;
}

} // namespace slag::vk
