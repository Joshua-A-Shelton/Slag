#include "d3d12/D3D12Formats.h"
#include "common/Formats.h"

namespace slag::d3d12 {

namespace {

// Indexed by PixelFormat.
constexpr DXGI_FORMAT dxgiFormats[] = {
    DXGI_FORMAT_UNKNOWN,

    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,

    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R16G16B16A16_FLOAT,

    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32A32_FLOAT,

    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,

    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,

    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
};
static_assert(sizeof(dxgiFormats) / sizeof(dxgiFormats[0]) == pixelFormatCount);

// Depth formats are sampled through a color format (docs/format-support.md).
DXGI_FORMAT sampledFormat(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::D16Unorm:
        return DXGI_FORMAT_R16_UNORM;
    case PixelFormat::D32Float:
        return DXGI_FORMAT_R32_FLOAT;
    case PixelFormat::D32FloatS8Uint:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    default:
        return dxgiFormats[static_cast<size_t>(format)];
    }
}

D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupportData(ID3D12Device* device, DXGI_FORMAT format)
{
    D3D12_FEATURE_DATA_FORMAT_SUPPORT data = {format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &data, sizeof(data))))
    {
        data.Support1 = D3D12_FORMAT_SUPPORT1_NONE;
        data.Support2 = D3D12_FORMAT_SUPPORT2_NONE;
    }
    return data;
}

bool multisampled(ID3D12Device* device, DXGI_FORMAT format, UINT count)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {format, count, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,
                                                            0};
    return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels))) &&
           levels.NumQualityLevels > 0;
}

} // namespace

DXGI_FORMAT toDxgiFormat(PixelFormat format)
{
    size_t index = static_cast<size_t>(format);
    return index < pixelFormatCount ? dxgiFormats[index] : DXGI_FORMAT_UNKNOWN;
}

void queryFormatSupport(ID3D12Device* device, FormatSupport* support)
{
    support[0] = FormatSupport::None;
    for (size_t i = 1; i < pixelFormatCount; ++i)
    {
        auto format = static_cast<PixelFormat>(i);
        D3D12_FEATURE_DATA_FORMAT_SUPPORT data = formatSupportData(device, dxgiFormats[i]);
        D3D12_FEATURE_DATA_FORMAT_SUPPORT sampled =
            sampledFormat(format) == dxgiFormats[i] ? data : formatSupportData(device, sampledFormat(format));
        auto has1 = [](const D3D12_FEATURE_DATA_FORMAT_SUPPORT& d, UINT bits) {
            return (static_cast<UINT>(d.Support1) & bits) == bits;
        };
        auto has2 = [](const D3D12_FEATURE_DATA_FORMAT_SUPPORT& d, UINT bits) {
            return (static_cast<UINT>(d.Support2) & bits) == bits;
        };

        FormatSupport result = FormatSupport::None;
        auto add = [&result](bool supported, FormatSupport bit) {
            if (supported)
            {
                result |= bit;
            }
        };
        add(has1(sampled, D3D12_FORMAT_SUPPORT1_SHADER_LOAD), FormatSupport::Sampled);
        add(has1(sampled, D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE), FormatSupport::LinearFilter);
        add(has1(data, D3D12_FORMAT_SUPPORT1_RENDER_TARGET), FormatSupport::RenderTarget);
        add(has1(data, D3D12_FORMAT_SUPPORT1_BLENDABLE), FormatSupport::Blend);
        add(has1(data, D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
                has2(data, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE),
            FormatSupport::RWTexture);
        add(has2(data, D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD), FormatSupport::RWTextureLoad);
        add(has2(data, D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD | D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS |
                           D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE |
                           D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_EXCHANGE |
                           D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_SIGNED_MIN_OR_MAX |
                           D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_UNSIGNED_MIN_OR_MAX),
            FormatSupport::Atomics);
        add(has1(data, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL), FormatSupport::DepthStencil);
        add(has1(data, D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER), FormatSupport::VertexBuffer);
        add(has1(data, D3D12_FORMAT_SUPPORT1_BUFFER | D3D12_FORMAT_SUPPORT1_SHADER_LOAD), FormatSupport::TypedBuffer);
        add(has1(data, D3D12_FORMAT_SUPPORT1_BUFFER | D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) &&
                has2(data, D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE),
            FormatSupport::RWTypedBuffer);
        if (has1(data, D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET) ||
            has1(data, D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL))
        {
            add(multisampled(device, dxgiFormats[i], 2), FormatSupport::Multisample2);
            add(multisampled(device, dxgiFormats[i], 4), FormatSupport::Multisample4);
            add(multisampled(device, dxgiFormats[i], 8), FormatSupport::Multisample8);
        }

        support[i] = result & formatInfo(format).allowed;
    }
}

} // namespace slag::d3d12
