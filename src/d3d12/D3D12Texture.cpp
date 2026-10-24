#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

namespace slag::d3d12 {

namespace {

constexpr size_t maxCastableFormats = 16;

// Depth textures are created typeless so they can also be sampled through a
// color format (docs/format-support.md).
DXGI_FORMAT resourceFormat(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::D16Unorm:
        return DXGI_FORMAT_R16_TYPELESS;
    case PixelFormat::D32Float:
        return DXGI_FORMAT_R32_TYPELESS;
    case PixelFormat::D32FloatS8Uint:
        return DXGI_FORMAT_R32G8X24_TYPELESS;
    default:
        return toDxgiFormat(format);
    }
}

struct ResourceInfo
{
    D3D12_RESOURCE_DESC1 desc = {};
    DXGI_FORMAT castable[maxCastableFormats] = {};
    UINT32 castableCount = 0;
    D3D12_CLEAR_VALUE clear = {};
    bool hasClear = false;
};

void fillResourceInfo(const TextureCreateInfo& info, ResourceInfo& out)
{
    D3D12_RESOURCE_DESC1& desc = out.desc;
    switch (info.type)
    {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
        break;
    case TextureType::Texture3D:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        break;
    default:
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // cubes are 2D arrays of 6 x N layers (ADR 0021)
        break;
    }
    desc.Width = info.width;
    desc.Height = info.height;
    desc.DepthOrArraySize = static_cast<UINT16>(info.type == TextureType::Texture3D ? info.depth : info.layerCount);
    desc.MipLevels = static_cast<UINT16>(info.mipCount);
    desc.Format = resourceFormat(info.format);
    desc.SampleDesc.Count = info.sampleCount;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    bool renderTarget = (info.usage & TextureUsage::RenderTarget) != TextureUsage::None;
    bool depthStencil = (info.usage & TextureUsage::DepthStencil) != TextureUsage::None;
    if (renderTarget)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (depthStencil)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if ((info.usage & TextureUsage::ShaderWrite) != TextureUsage::None)
    {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Relaxed format casting (ADR 0022).
    for (PixelFormat format : info.castableFormats)
    {
        if (out.castableCount < maxCastableFormats)
        {
            out.castable[out.castableCount++] = toDxgiFormat(format);
        }
    }

    // Optimized clear value, in the format targets are viewed with (ADR 0021).
    if (info.clearValue && (renderTarget || depthStencil))
    {
        out.hasClear = true;
        out.clear.Format = toDxgiFormat(info.format);
        if (depthStencil)
        {
            out.clear.DepthStencil.Depth = info.clearValue->depth;
            out.clear.DepthStencil.Stencil = info.clearValue->stencil;
        }
        else
        {
            for (int i = 0; i < 4; ++i)
            {
                out.clear.Color[i] = info.clearValue->color[i];
            }
        }
    }
}

D3D12_RESOURCE_ALLOCATION_INFO allocationInfo(ID3D12Device12* device, const ResourceInfo& info,
                                              D3D12_RESOURCE_DESC1 desc)
{
    const DXGI_FORMAT* castable = info.castable;
    return device->GetResourceAllocationInfo3(0, 1, &desc, &info.castableCount, &castable, nullptr);
}

// 4 KB alignment for small textures when the device allows it, else the
// default (64 KB, or 4 MB for MSAA) (ADR 0023).
D3D12_RESOURCE_ALLOCATION_INFO placement(ID3D12Device12* device, ResourceInfo& info)
{
    bool target =
        (info.desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0;
    if (!target && info.desc.SampleDesc.Count == 1)
    {
        D3D12_RESOURCE_DESC1 small = info.desc;
        small.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
        D3D12_RESOURCE_ALLOCATION_INFO result = allocationInfo(device, info, small);
        if (result.Alignment == D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT)
        {
            info.desc.Alignment = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
            return result;
        }
    }
    info.desc.Alignment = 0;
    return allocationInfo(device, info, info.desc);
}

} // namespace

Result<TextureCommon*> createTexture(GraphicsDeviceCommon* common, const TextureCreateInfo& info)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* texture = device.instance->allocator.create<TextureImpl>();
    if (!texture)
    {
        return Status::OutOfHostMemory;
    }
    texture->device = &device;
    texture->functions = device.functions;
    texture->planeCount = hasTraits(info.format, FormatTraits::Stencil) ? 2 : 1;

    ResourceInfo resource;
    fillResourceInfo(info, resource);
    const D3D12_CLEAR_VALUE* clear = resource.hasClear ? &resource.clear : nullptr;
    const DXGI_FORMAT* castable = resource.castableCount ? resource.castable : nullptr;
    HRESULT result;
    if (info.placement.memoryBlock)
    {
        // The same alignment the placement query chose.
        (void)placement(device.device, resource);
        auto* block = static_cast<MemoryBlockImpl*>(info.placement.memoryBlock);
        result = device.device->CreatePlacedResource2(block->heap, info.placement.offset, &resource.desc,
                                                      D3D12_BARRIER_LAYOUT_UNDEFINED, clear, resource.castableCount,
                                                      castable, IID_PPV_ARGS(&texture->resource));
    }
    else
    {
        D3D12_HEAP_PROPERTIES heap = {};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        // Contents start undefined (ADR 0021).
        result = device.device->CreateCommittedResource3(
            &heap, D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, &resource.desc, D3D12_BARRIER_LAYOUT_UNDEFINED, clear, nullptr,
            resource.castableCount, castable, IID_PPV_ARGS(&texture->resource));
    }
    if (FAILED(result))
    {
        device.instance->allocator.destroy(texture);
        return toStatus(device, result);
    }
    setObjectName(texture->resource, info.debugName);
    return static_cast<TextureCommon*>(texture);
}

void destroyTexture(GraphicsDeviceCommon* common, TextureCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* texture = static_cast<TextureImpl*>(object);
    release(texture->resource);
    device.instance->allocator.destroy(texture);
}

PlacementInfo texturePlacementInfo(const GraphicsDeviceCommon* common, const TextureCreateInfo& info)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    ResourceInfo resource;
    fillResourceInfo(info, resource);
    D3D12_RESOURCE_ALLOCATION_INFO result = placement(device.device, resource);
    return {result.SizeInBytes, result.Alignment};
}

} // namespace slag::d3d12
