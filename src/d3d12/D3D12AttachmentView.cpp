#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

namespace slag::d3d12 {

namespace {

constexpr uint32_t groupsPerHeap = 256;

D3D12_RENDER_TARGET_VIEW_DESC rtvDesc(const TextureCommon& texture, const AttachmentViewDesc& desc)
{
    D3D12_RENDER_TARGET_VIEW_DESC view = {};
    view.Format = toDxgiFormat(desc.format);
    bool msaa = texture.sampleCount > 1;
    switch (texture.type)
    {
    case TextureType::Texture1D:
        view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        view.Texture1D.MipSlice = desc.mip;
        break;
    case TextureType::Texture1DArray:
        view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        view.Texture1DArray = {desc.mip, desc.baseLayer, desc.layerCount};
        break;
    case TextureType::Texture2D:
        if (msaa)
        {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            view.Texture2D = {desc.mip, 0};
        }
        break;
    case TextureType::Texture2DArray:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        if (msaa)
        {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
            view.Texture2DMSArray = {desc.baseLayer, desc.layerCount};
        }
        else
        {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
            view.Texture2DArray = {desc.mip, desc.baseLayer, desc.layerCount, 0};
        }
        break;
    case TextureType::Texture3D:
        view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        view.Texture3D = {desc.mip, desc.baseLayer, desc.layerCount};
        break;
    }
    return view;
}

D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc(const TextureCommon& texture, const AttachmentViewDesc& desc,
                                      D3D12_DSV_FLAGS flags)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC view = {};
    view.Format = toDxgiFormat(desc.format);
    view.Flags = flags;
    bool msaa = texture.sampleCount > 1;
    if (texture.type == TextureType::Texture2D)
    {
        if (msaa)
        {
            view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        else
        {
            view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            view.Texture2D.MipSlice = desc.mip;
        }
    }
    else if (msaa)
    {
        view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
        view.Texture2DMSArray = {desc.baseLayer, desc.layerCount};
    }
    else
    {
        view.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY; // 2D arrays and cubes
        view.Texture2DArray = {desc.mip, desc.baseLayer, desc.layerCount};
    }
    return view;
}

} // namespace

void initPool(GraphicsDeviceImpl& device, CpuDescriptorPool& pool, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t groupSize)
{
    pool.type = type;
    pool.groupSize = groupSize;
    pool.increment = device.device->GetDescriptorHandleIncrementSize(type);
    pool.heaps = Vector<ID3D12DescriptorHeap*>(&device.instance->allocator);
    pool.freeGroups = Vector<SIZE_T>(&device.instance->allocator);
}

void destroyPool(CpuDescriptorPool& pool)
{
    for (ID3D12DescriptorHeap* heap : pool.heaps)
    {
        heap->Release();
    }
    pool.heaps.clear();
    pool.freeGroups.clear();
}

Status allocateGroup(GraphicsDeviceImpl& device, CpuDescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE& handle)
{
    std::lock_guard<std::mutex> lock(pool.lock);
    if (pool.freeGroups.empty())
    {
        // A new CPU-only heap, split into groups.
        if (!pool.heaps.reserve(pool.heaps.size() + 1) ||
            !pool.freeGroups.reserve(pool.freeGroups.size() + groupsPerHeap))
        {
            return Status::OutOfHostMemory;
        }
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = pool.type;
        desc.NumDescriptors = groupsPerHeap * pool.groupSize;
        ID3D12DescriptorHeap* heap = nullptr;
        HRESULT result = device.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
        if (FAILED(result))
        {
            return toStatus(device, result);
        }
        (void)pool.heaps.pushBack(heap);
        SIZE_T start = heap->GetCPUDescriptorHandleForHeapStart().ptr;
        for (uint32_t i = groupsPerHeap; i > 0; --i)
        {
            (void)pool.freeGroups.pushBack(start + SIZE_T(i - 1) * pool.groupSize * pool.increment);
        }
    }
    handle.ptr = pool.freeGroups[pool.freeGroups.size() - 1];
    pool.freeGroups.popBack();
    return Status::Ok;
}

void freeGroup(CpuDescriptorPool& pool, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    std::lock_guard<std::mutex> lock(pool.lock);
    (void)pool.freeGroups.pushBack(handle.ptr); // capacity covers every group
}

Result<AttachmentViewCommon*> createAttachmentView(GraphicsDeviceCommon* common, const AttachmentViewDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* texture = static_cast<TextureImpl*>(desc.texture);
    auto* view = device.instance->allocator.create<AttachmentViewImpl>();
    if (!view)
    {
        return Status::OutOfHostMemory;
    }
    view->device = &device;
    view->functions = device.functions;
    view->depthStencil = (texture->usage & TextureUsage::DepthStencil) != TextureUsage::None;

    CpuDescriptorPool& pool = view->depthStencil ? device.dsvPool : device.rtvPool;
    Status status = allocateGroup(device, pool, view->handle);
    if (status != Status::Ok)
    {
        device.instance->allocator.destroy(view);
        return status;
    }
    if (view->depthStencil)
    {
        // Every read-only variant, so rendering can pick one (ADR 0035).
        bool stencil = texture->planeCount > 1;
        const D3D12_DSV_FLAGS flags[DsvVariantCount] = {
            D3D12_DSV_FLAG_NONE, D3D12_DSV_FLAG_READ_ONLY_DEPTH,
            stencil ? D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_NONE,
            stencil ? D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL
                    : D3D12_DSV_FLAG_READ_ONLY_DEPTH};
        for (uint32_t i = 0; i < DsvVariantCount; ++i)
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv = dsvDesc(*texture, desc, flags[i]);
            D3D12_CPU_DESCRIPTOR_HANDLE handle = {view->handle.ptr + SIZE_T(i) * pool.increment};
            device.device->CreateDepthStencilView(texture->resource, &dsv, handle);
        }
    }
    else
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtv = rtvDesc(*texture, desc);
        device.device->CreateRenderTargetView(texture->resource, &rtv, view->handle);
    }
    return static_cast<AttachmentViewCommon*>(view);
}

void destroyAttachmentView(GraphicsDeviceCommon* common, AttachmentViewCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* view = static_cast<AttachmentViewImpl*>(object);
    freeGroup(view->depthStencil ? device.dsvPool : device.rtvPool, view->handle);
    device.instance->allocator.destroy(view);
}

} // namespace slag::d3d12
