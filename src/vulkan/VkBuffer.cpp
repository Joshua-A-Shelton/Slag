#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkMemory.h"

namespace slag::vk {

namespace {

// Every usage, so any buffer can be used for anything (ADR 0020).
constexpr VkBufferUsageFlags allBufferUsage =
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
    VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

struct MemoryFlags
{
    VkMemoryPropertyFlags required;
    VkMemoryPropertyFlags preferred;
    VkMemoryPropertyFlags avoided;
};

// ADR 0019. Upload and Readback stay in system memory, like D3D12's heaps.
MemoryFlags memoryFlags(MemoryKind kind)
{
    constexpr VkMemoryPropertyFlags hostCoherent =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    switch (kind)
    {
    case MemoryKind::GpuOnly:
        return {VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT};
    case MemoryKind::Upload:
        return {hostCoherent, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    case MemoryKind::Readback:
        return {hostCoherent, VK_MEMORY_PROPERTY_HOST_CACHED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
    case MemoryKind::GpuUpload:
        return {hostCoherent | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0};
    }
    return {};
}

// GpuUpload needs full resizable BAR: a CPU-visible device-local heap about the
// size of VRAM, not the 256 MB window (ADR 0019).
bool supportsGpuUpload(const GraphicsDeviceImpl& device, uint32_t memoryType)
{
    uint32_t heapIndex = device.memoryProperties.memoryTypes[memoryType].heapIndex;
    VkDeviceSize heapSize = device.memoryProperties.memoryHeaps[heapIndex].size;
    return heapSize * 10 >= device.info.dedicatedMemoryBytes * 9;
}

} // namespace

Result<BufferCommon*> createBuffer(GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* buffer = device.instance->allocator.create<BufferImpl>();
    if (!buffer)
    {
        return Status::OutOfHostMemory;
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
        if (status == Status::Unsupported)
        {
            logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                       "slag: GraphicsDevice::createBuffer: no suitable memory for this memory kind on \"%s\"",
                       device.info.name);
        }
        device.instance->allocator.destroy(buffer);
        return status;
    }

    buffer->device = &device;
    buffer->functions = device.functions;
    buffer->size = desc.size;
    buffer->kind = desc.placement.kind;
    buffer->buffer = allocation.buffer;
    buffer->memory = allocation.memory;
    buffer->mapped = desc.placement.kind == MemoryKind::GpuOnly ? nullptr : allocation.mapped;
    buffer->gpuAddress = allocation.address;
    setObjectName(device, VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(buffer->buffer), desc.debugName);
    return static_cast<BufferCommon*>(buffer);
}

void destroyBuffer(GraphicsDeviceCommon* common, BufferCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* buffer = static_cast<BufferImpl*>(object);
    DedicatedBuffer allocation;
    allocation.buffer = buffer->buffer;
    allocation.memory = buffer->memory;
    allocation.mapped = buffer->mapped;
    destroyDedicatedBuffer(device, allocation);
    device.instance->allocator.destroy(buffer);
}

} // namespace slag::vk
