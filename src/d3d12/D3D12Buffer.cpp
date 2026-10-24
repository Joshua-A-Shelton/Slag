#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

namespace {

// ADR 0019.
D3D12_HEAP_TYPE heapType(MemoryKind kind)
{
    switch (kind)
    {
    case MemoryKind::GpuOnly:
        return D3D12_HEAP_TYPE_DEFAULT;
    case MemoryKind::Upload:
        return D3D12_HEAP_TYPE_UPLOAD;
    case MemoryKind::Readback:
        return D3D12_HEAP_TYPE_READBACK;
    case MemoryKind::GpuUpload:
        return D3D12_HEAP_TYPE_GPU_UPLOAD;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_RESOURCE_DESC1 bufferDesc(uint64_t size, MemoryKind kind)
{
    D3D12_RESOURCE_DESC1 desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    // Every usage, so any buffer can be used for anything (ADR 0020). Upload
    // and readback heaps don't allow UAVs.
    if (kind == MemoryKind::GpuOnly || kind == MemoryKind::GpuUpload)
    {
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return desc;
}

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

} // namespace

Result<MemoryBlockCommon*> createMemoryBlock(GraphicsDeviceCommon* common, const MemoryBlockDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* block = device.instance->allocator.create<MemoryBlockImpl>();
    if (!block)
    {
        return Status::OutOfHostMemory;
    }
    block->device = &device;
    block->functions = device.functions;
    block->size = desc.size;
    block->kind = desc.kind;

    // 4 MB-aligned so MSAA textures fit (ADR 0023). Heap sizes should be a
    // multiple of the alignment, so blocks under 4 MB use 64 KB instead.
    D3D12_HEAP_DESC heapDesc = {};
    heapDesc.Alignment = desc.size >= D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
                             ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT
                             : D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    heapDesc.SizeInBytes = alignUp(desc.size, heapDesc.Alignment);
    heapDesc.Properties.Type = heapType(desc.kind);
    // Resource heap tier 2: any resource type in one GpuOnly heap (ADR 0023).
    // CPU-visible kinds hold buffers only.
    heapDesc.Flags = D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    if (desc.kind != MemoryKind::GpuOnly)
    {
        heapDesc.Flags |= D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
    }
    HRESULT result = device.device->CreateHeap(&heapDesc, IID_PPV_ARGS(&block->heap));
    if (FAILED(result))
    {
        device.instance->allocator.destroy(block);
        return toStatus(device, result);
    }
    setObjectName(block->heap, desc.debugName);
    return static_cast<MemoryBlockCommon*>(block);
}

void destroyMemoryBlock(GraphicsDeviceCommon* common, MemoryBlockCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* block = static_cast<MemoryBlockImpl*>(object);
    release(block->heap);
    device.instance->allocator.destroy(block);
}

Result<BufferCommon*> createBuffer(GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* block = static_cast<MemoryBlockImpl*>(desc.placement.memoryBlock);
    MemoryKind kind = block ? block->kind : desc.placement.kind;
    if (kind == MemoryKind::GpuUpload && !device.capabilities.gpuUploadMemory)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createBuffer: no suitable memory for this memory kind on \"%s\"",
                   device.info.name);
        return Status::Unsupported;
    }
    auto* buffer = device.instance->allocator.create<BufferImpl>();
    if (!buffer)
    {
        return Status::OutOfHostMemory;
    }

    D3D12_RESOURCE_DESC1 resource = bufferDesc(desc.size, kind);
    HRESULT result;
    if (block)
    {
        result = device.device->CreatePlacedResource2(block->heap, desc.placement.offset, &resource,
                                                      D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr,
                                                      IID_PPV_ARGS(&buffer->resource));
    }
    else
    {
        D3D12_HEAP_PROPERTIES heap = {};
        heap.Type = heapType(kind);
        result = device.device->CreateCommittedResource3(&heap, D3D12_HEAP_FLAG_NONE, &resource,
                                                         D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, nullptr, 0, nullptr,
                                                         IID_PPV_ARGS(&buffer->resource));
    }
    void* mapped = nullptr;
    if (SUCCEEDED(result) && kind != MemoryKind::GpuOnly)
    {
        // Mapped for the buffer's lifetime (ADR 0024). The CPU only reads readback memory.
        D3D12_RANGE noRead = {0, 0};
        result = buffer->resource->Map(0, kind == MemoryKind::Readback ? nullptr : &noRead, &mapped);
    }
    if (FAILED(result))
    {
        release(buffer->resource);
        device.instance->allocator.destroy(buffer);
        return toStatus(device, result);
    }

    buffer->device = &device;
    buffer->functions = device.functions;
    buffer->size = desc.size;
    buffer->kind = kind;
    buffer->mapped = mapped;
    buffer->gpuAddress = buffer->resource->GetGPUVirtualAddress();
    setObjectName(buffer->resource, desc.debugName);
    return static_cast<BufferCommon*>(buffer);
}

void destroyBuffer(GraphicsDeviceCommon* common, BufferCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* buffer = static_cast<BufferImpl*>(object);
    release(buffer->resource); // unmaps
    device.instance->allocator.destroy(buffer);
}

PlacementInfo bufferPlacementInfo(const GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    D3D12_RESOURCE_DESC1 resource = bufferDesc(desc.size, MemoryKind::GpuOnly);
    D3D12_RESOURCE_ALLOCATION_INFO info = device.device->GetResourceAllocationInfo2(0, 1, &resource, nullptr);
    return {info.SizeInBytes, info.Alignment};
}

} // namespace slag::d3d12
