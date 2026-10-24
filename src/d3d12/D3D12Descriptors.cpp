#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

namespace slag::d3d12 {

namespace {

GraphicsDeviceImpl& deviceOf(GraphicsDeviceCommon* common)
{
    return *static_cast<GraphicsDeviceImpl*>(common);
}

D3D12_CPU_DESCRIPTOR_HANDLE slot(const DescriptorHeap& heap, uint32_t index)
{
    return {heap.cpuStart.ptr + SIZE_T(index) * heap.increment};
}

// What a plane of a texture is viewed as: depth resources are typeless
// (D3D12Texture.cpp, docs/format-support.md).
DXGI_FORMAT viewFormat(PixelFormat format, TexturePlane plane)
{
    switch (format)
    {
    case PixelFormat::D16Unorm:
        return DXGI_FORMAT_R16_UNORM;
    case PixelFormat::D32Float:
        return DXGI_FORMAT_R32_FLOAT;
    case PixelFormat::D32FloatS8Uint:
        return plane == TexturePlane::Stencil ? DXGI_FORMAT_X32_TYPELESS_G8X24_UINT
                                              : DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    default:
        return toDxgiFormat(format);
    }
}

D3D12_FILTER_TYPE filterType(Filter filter)
{
    return filter == Filter::Linear ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT;
}

D3D12_TEXTURE_ADDRESS_MODE addressMode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::Wrap:
        return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case AddressMode::Mirror:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case AddressMode::Clamp:
        return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::Border:
        return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    case AddressMode::MirrorOnce:
        return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
    }
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
}

} // namespace

D3D12_COMPARISON_FUNC toComparisonFunc(CompareOp op)
{
    static_assert(D3D12_COMPARISON_FUNC_NEVER == 1 && D3D12_COMPARISON_FUNC_ALWAYS == 8);
    return static_cast<D3D12_COMPARISON_FUNC>(static_cast<int>(op) + 1); // same order, starting at 1
}

void writeBufferDescriptor(GraphicsDeviceCommon* common, uint32_t index, const BufferDescriptorInfo& info)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    D3D12_CPU_DESCRIPTOR_HANDLE destination = slot(device.resourceHeap, index);
    ID3D12Resource* resource = info.buffer ? static_cast<BufferImpl*>(info.buffer)->resource : nullptr;
    switch (info.kind)
    {
    case BufferDescriptorKind::Constant:
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC view = {info.address, static_cast<UINT>(info.size)};
        device.device->CreateConstantBufferView(&view, destination);
        return;
    }
    case BufferDescriptorKind::Raw:
    case BufferDescriptorKind::Structured:
    case BufferDescriptorKind::Typed:
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
        view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        if (info.kind == BufferDescriptorKind::Raw)
        {
            view.Format = DXGI_FORMAT_R32_TYPELESS;
            view.Buffer.FirstElement = info.offset / 4;
            view.Buffer.NumElements = static_cast<UINT>(info.size / 4);
            view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
        }
        else if (info.kind == BufferDescriptorKind::Structured)
        {
            view.Format = DXGI_FORMAT_UNKNOWN;
            view.Buffer.FirstElement = info.offset / info.stride; // validated as a multiple
            view.Buffer.NumElements = info.count;
            view.Buffer.StructureByteStride = info.stride;
        }
        else
        {
            view.Format = toDxgiFormat(info.format);
            view.Buffer.FirstElement = info.offset / formatInfo(info.format).blockBytes;
            view.Buffer.NumElements = info.count;
        }
        device.device->CreateShaderResourceView(resource, &view, destination);
        return;
    }
    case BufferDescriptorKind::RWRaw:
    case BufferDescriptorKind::RWStructured:
    case BufferDescriptorKind::RWTyped:
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
        view.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        if (info.kind == BufferDescriptorKind::RWRaw)
        {
            view.Format = DXGI_FORMAT_R32_TYPELESS;
            view.Buffer.FirstElement = info.offset / 4;
            view.Buffer.NumElements = static_cast<UINT>(info.size / 4);
            view.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        }
        else if (info.kind == BufferDescriptorKind::RWStructured)
        {
            view.Format = DXGI_FORMAT_UNKNOWN;
            view.Buffer.FirstElement = info.offset / info.stride;
            view.Buffer.NumElements = info.count;
            view.Buffer.StructureByteStride = info.stride;
        }
        else
        {
            view.Format = toDxgiFormat(info.format);
            view.Buffer.FirstElement = info.offset / formatInfo(info.format).blockBytes;
            view.Buffer.NumElements = info.count;
        }
        device.device->CreateUnorderedAccessView(resource, nullptr, &view, destination);
        return;
    }
    }
}

void writeTextureDescriptor(GraphicsDeviceCommon* common, uint32_t index, const TextureDescriptorInfo& info)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    auto* texture = static_cast<TextureImpl*>(info.texture);
    D3D12_CPU_DESCRIPTOR_HANDLE destination = slot(device.resourceHeap, index);
    UINT plane = info.plane == TexturePlane::Stencil ? 1 : 0;

    if (info.writable)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC view = {};
        view.Format = toDxgiFormat(info.format);
        switch (info.viewType)
        {
        case TextureViewType::Texture1D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
            view.Texture1D.MipSlice = info.baseMip;
            break;
        case TextureViewType::Texture1DArray:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            view.Texture1DArray = {info.baseMip, info.baseLayer, info.layerCount};
            break;
        case TextureViewType::Texture2D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            view.Texture2D = {info.baseMip, 0};
            break;
        case TextureViewType::Texture3D:
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
            view.Texture3D = {info.baseMip, 0, UINT(-1)}; // whole depth (RWTextureDescriptor)
            break;
        default: // 2D arrays and cubes: writable cubes are 2D arrays in HLSL
            view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
            view.Texture2DArray = {info.baseMip, info.baseLayer, info.layerCount, 0};
            break;
        }
        device.device->CreateUnorderedAccessView(texture->resource, nullptr, &view, destination);
        return;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
    view.Format = hasTraits(texture->format, FormatTraits::Depth) ? viewFormat(texture->format, info.plane)
                                                                  : toDxgiFormat(info.format);
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (info.viewType)
    {
    case TextureViewType::Texture1D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        view.Texture1D = {info.baseMip, info.mipCount, 0.0f};
        break;
    case TextureViewType::Texture1DArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        view.Texture1DArray = {info.baseMip, info.mipCount, info.baseLayer, info.layerCount, 0.0f};
        break;
    case TextureViewType::Texture2D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        view.Texture2D = {info.baseMip, info.mipCount, plane, 0.0f};
        break;
    case TextureViewType::Texture2DArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        view.Texture2DArray = {info.baseMip, info.mipCount, info.baseLayer, info.layerCount, plane, 0.0f};
        break;
    case TextureViewType::Texture2DMS:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        break;
    case TextureViewType::Texture2DMSArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        view.Texture2DMSArray = {info.baseLayer, info.layerCount};
        break;
    case TextureViewType::Texture3D:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        view.Texture3D = {info.baseMip, info.mipCount, 0.0f};
        break;
    case TextureViewType::TextureCube:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        view.TextureCube = {info.baseMip, info.mipCount, 0.0f};
        break;
    case TextureViewType::TextureCubeArray:
        view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        view.TextureCubeArray = {info.baseMip, info.mipCount, info.baseLayer, info.layerCount / 6, 0.0f};
        break;
    }
    device.device->CreateShaderResourceView(texture->resource, &view, destination);
}

void writeSamplerDescriptor(GraphicsDeviceCommon* common, uint32_t index, const SamplerDescriptor& descriptor)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    D3D12_FILTER_REDUCTION_TYPE reduction = D3D12_FILTER_REDUCTION_TYPE_STANDARD;
    if (descriptor.compareEnable)
    {
        reduction = D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
    }
    else if (descriptor.reductionMode == ReductionMode::Minimum)
    {
        reduction = D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
    }
    else if (descriptor.reductionMode == ReductionMode::Maximum)
    {
        reduction = D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
    }
    D3D12_SAMPLER_DESC sampler = {};
    sampler.Filter = descriptor.maxAnisotropy > 1
                         ? D3D12_ENCODE_ANISOTROPIC_FILTER(reduction)
                         : D3D12_ENCODE_BASIC_FILTER(filterType(descriptor.minFilter), filterType(descriptor.magFilter),
                                                     filterType(descriptor.mipFilter), reduction);
    sampler.AddressU = addressMode(descriptor.addressU);
    sampler.AddressV = addressMode(descriptor.addressV);
    sampler.AddressW = addressMode(descriptor.addressW);
    sampler.MipLODBias = descriptor.mipLodBias;
    sampler.MaxAnisotropy = descriptor.maxAnisotropy;
    sampler.ComparisonFunc =
        descriptor.compareEnable ? toComparisonFunc(descriptor.compareOp) : D3D12_COMPARISON_FUNC_NEVER;
    float alpha = descriptor.borderColor == BorderColor::TransparentBlack ? 0.0f : 1.0f;
    float color = descriptor.borderColor == BorderColor::OpaqueWhite ? 1.0f : 0.0f;
    sampler.BorderColor[0] = color;
    sampler.BorderColor[1] = color;
    sampler.BorderColor[2] = color;
    sampler.BorderColor[3] = alpha;
    sampler.MinLOD = descriptor.minLod;
    sampler.MaxLOD = descriptor.maxLod;
    device.device->CreateSampler(&sampler, slot(device.samplerHeap, index));
}

} // namespace slag::d3d12
