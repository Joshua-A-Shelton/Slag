#include "vulkan/VkFormats.h"
#include "common/Formats.h"

namespace slag::vk {

namespace {

// Indexed by PixelFormat. Packed formats map by bit layout (ADR 0022).
constexpr VkFormat vkFormats[] = {
    VK_FORMAT_UNDEFINED,

    VK_FORMAT_R8_UNORM,
    VK_FORMAT_R8_SNORM,
    VK_FORMAT_R8_UINT,
    VK_FORMAT_R8_SINT,
    VK_FORMAT_R8G8_UNORM,
    VK_FORMAT_R8G8_SNORM,
    VK_FORMAT_R8G8_UINT,
    VK_FORMAT_R8G8_SINT,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_R8G8B8A8_SNORM,
    VK_FORMAT_R8G8B8A8_UINT,
    VK_FORMAT_R8G8B8A8_SINT,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB,

    VK_FORMAT_R16_UNORM,
    VK_FORMAT_R16_SNORM,
    VK_FORMAT_R16_UINT,
    VK_FORMAT_R16_SINT,
    VK_FORMAT_R16_SFLOAT,
    VK_FORMAT_R16G16_UNORM,
    VK_FORMAT_R16G16_SNORM,
    VK_FORMAT_R16G16_UINT,
    VK_FORMAT_R16G16_SINT,
    VK_FORMAT_R16G16_SFLOAT,
    VK_FORMAT_R16G16B16A16_UNORM,
    VK_FORMAT_R16G16B16A16_SNORM,
    VK_FORMAT_R16G16B16A16_UINT,
    VK_FORMAT_R16G16B16A16_SINT,
    VK_FORMAT_R16G16B16A16_SFLOAT,

    VK_FORMAT_R32_UINT,
    VK_FORMAT_R32_SINT,
    VK_FORMAT_R32_SFLOAT,
    VK_FORMAT_R32G32_UINT,
    VK_FORMAT_R32G32_SINT,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_UINT,
    VK_FORMAT_R32G32B32_SINT,
    VK_FORMAT_R32G32B32_SFLOAT,
    VK_FORMAT_R32G32B32A32_UINT,
    VK_FORMAT_R32G32B32A32_SINT,
    VK_FORMAT_R32G32B32A32_SFLOAT,

    VK_FORMAT_A2B10G10R10_UNORM_PACK32,
    VK_FORMAT_A2B10G10R10_UINT_PACK32,
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,

    VK_FORMAT_D16_UNORM,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,

    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
    VK_FORMAT_BC3_UNORM_BLOCK,
    VK_FORMAT_BC3_SRGB_BLOCK,
    VK_FORMAT_BC4_UNORM_BLOCK,
    VK_FORMAT_BC4_SNORM_BLOCK,
    VK_FORMAT_BC5_UNORM_BLOCK,
    VK_FORMAT_BC5_SNORM_BLOCK,
    VK_FORMAT_BC6H_UFLOAT_BLOCK,
    VK_FORMAT_BC6H_SFLOAT_BLOCK,
    VK_FORMAT_BC7_UNORM_BLOCK,
    VK_FORMAT_BC7_SRGB_BLOCK,
};
static_assert(sizeof(vkFormats) / sizeof(vkFormats[0]) == pixelFormatCount);

// MSAA counts for one attachment usage; 0 when the format can't be one.
VkSampleCountFlags sampleCounts(const InstanceImpl& instance, VkPhysicalDevice device, VkFormat format,
                                VkImageUsageFlags usage)
{
    VkPhysicalDeviceImageFormatInfo2 info = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2};
    info.format = format;
    info.type = VK_IMAGE_TYPE_2D;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    VkImageFormatProperties2 properties = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
    if (instance.fn.vkGetPhysicalDeviceImageFormatProperties2(device, &info, &properties) != VK_SUCCESS)
    {
        return 0;
    }
    return properties.imageFormatProperties.sampleCounts;
}

} // namespace

VkFormat toVkFormat(PixelFormat format)
{
    size_t index = static_cast<size_t>(format);
    return index < pixelFormatCount ? vkFormats[index] : VK_FORMAT_UNDEFINED;
}

void queryFormatSupport(const InstanceImpl& instance, VkPhysicalDevice device, FormatSupport* support)
{
    support[0] = FormatSupport::None;
    for (size_t i = 1; i < pixelFormatCount; ++i)
    {
        auto format = static_cast<PixelFormat>(i);
        VkFormatProperties3 properties3 = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_3};
        VkFormatProperties2 properties = {VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        properties.pNext = &properties3;
        instance.fn.vkGetPhysicalDeviceFormatProperties2(device, vkFormats[i], &properties);
        VkFormatFeatureFlags2 image = properties3.optimalTilingFeatures;
        VkFormatFeatureFlags2 buffer = properties3.bufferFeatures;

        auto has = [](VkFormatFeatureFlags2 features, VkFormatFeatureFlags2 bits) {
            return (features & bits) == bits;
        };
        FormatSupport result = FormatSupport::None;
        auto add = [&result](bool supported, FormatSupport bit) {
            if (supported)
            {
                result |= bit;
            }
        };
        add(has(image, VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT), FormatSupport::Sampled);
        add(has(image, VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT), FormatSupport::LinearFilter);
        add(has(image, VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT), FormatSupport::RenderTarget);
        add(has(image, VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT), FormatSupport::Blend);
        add(has(image, VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT), FormatSupport::RWTexture);
        add(has(image, VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT), FormatSupport::RWTextureLoad);
        add(has(image, VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT) &&
                has(buffer, VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_ATOMIC_BIT),
            FormatSupport::Atomics);
        add(has(image, VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT), FormatSupport::DepthStencil);
        add(has(buffer, VK_FORMAT_FEATURE_2_VERTEX_BUFFER_BIT), FormatSupport::VertexBuffer);
        add(has(buffer, VK_FORMAT_FEATURE_2_UNIFORM_TEXEL_BUFFER_BIT), FormatSupport::TypedBuffer);
        add(has(buffer, VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT), FormatSupport::RWTypedBuffer);

        VkSampleCountFlags counts = 0;
        if ((result & FormatSupport::DepthStencil) != FormatSupport::None)
        {
            counts = sampleCounts(instance, device, vkFormats[i], VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
        }
        else if ((result & FormatSupport::RenderTarget) != FormatSupport::None)
        {
            counts = sampleCounts(instance, device, vkFormats[i], VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        }
        add((counts & VK_SAMPLE_COUNT_2_BIT) != 0, FormatSupport::Multisample2);
        add((counts & VK_SAMPLE_COUNT_4_BIT) != 0, FormatSupport::Multisample4);
        add((counts & VK_SAMPLE_COUNT_8_BIT) != 0, FormatSupport::Multisample8);

        support[i] = result & formatInfo(format).allowed;
    }
}

} // namespace slag::vk
