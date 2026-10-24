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

} // namespace

Result<BufferCommon*> createBuffer(GraphicsDeviceCommon* common, const BufferDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    if (desc.placement.kind == MemoryKind::GpuUpload && !device.gpuUploadHeap)
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

    D3D12_HEAP_PROPERTIES heap = {};
    heap.Type = heapType(desc.placement.kind);
    D3D12_RESOURCE_DESC1 resource = {};
    resource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource.Width = desc.size;
    resource.Height = 1;
    resource.DepthOrArraySize = 1;
    resource.MipLevels = 1;
    resource.Format = DXGI_FORMAT_UNKNOWN;
    resource.SampleDesc.Count = 1;
    resource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    // Every usage, so any buffer can be used for anything (ADR 0020). Upload
    // and readback heaps don't allow UAVs.
    if (heap.Type == D3D12_HEAP_TYPE_DEFAULT || heap.Type == D3D12_HEAP_TYPE_GPU_UPLOAD)
    {
        resource.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    HRESULT result =
        device.device->CreateCommittedResource3(&heap, D3D12_HEAP_FLAG_NONE, &resource, D3D12_BARRIER_LAYOUT_UNDEFINED,
                                                nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&buffer->resource));
    void* mapped = nullptr;
    if (SUCCEEDED(result) && desc.placement.kind != MemoryKind::GpuOnly)
    {
        // Mapped for the buffer's lifetime. The CPU only reads readback memory.
        D3D12_RANGE noRead = {0, 0};
        result = buffer->resource->Map(0, desc.placement.kind == MemoryKind::Readback ? nullptr : &noRead, &mapped);
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
    buffer->kind = desc.placement.kind;
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

} // namespace slag::d3d12
