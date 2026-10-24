#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"
#include "vulkan/VkMemory.h"

namespace slag::vk {

namespace {

constexpr size_t maxCastableFormats = 16;

// D3D12's texture placement alignments, on both backends (ADR 0023).
constexpr VkDeviceSize smallTextureAlignment = 4096;
constexpr VkDeviceSize textureAlignment = 65536;
constexpr VkDeviceSize msaaAlignment = 4u << 20;

// The create info plus storage for its format list.
struct ImageInfo
{
    VkImageCreateInfo create = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VkImageFormatListCreateInfo formatList = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
    VkFormat formats[maxCastableFormats + 1] = {};
};

void fillImageInfo(const GraphicsDeviceImpl& device, const TextureCreateInfo& info, ImageInfo& out)
{
    VkImageCreateInfo& create = out.create;
    switch (info.type)
    {
    case TextureType::Texture1D:
    case TextureType::Texture1DArray:
        create.imageType = VK_IMAGE_TYPE_1D;
        break;
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
        create.imageType = VK_IMAGE_TYPE_2D;
        break;
    case TextureType::Texture3D:
        create.imageType = VK_IMAGE_TYPE_3D;
        // Render targets on 3D textures are slice ranges (ADR 0021).
        if ((info.usage & TextureUsage::RenderTarget) != TextureUsage::None)
        {
            create.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        }
        break;
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        create.imageType = VK_IMAGE_TYPE_2D;
        create.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    }
    create.format = toVkFormat(info.format);
    create.extent = {info.width, info.height, info.depth};
    create.mipLevels = info.mipCount;
    create.arrayLayers = info.layerCount;
    create.samples = static_cast<VkSampleCountFlagBits>(info.sampleCount);
    create.tiling = VK_IMAGE_TILING_OPTIMAL;
    create.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((info.usage & TextureUsage::RenderTarget) != TextureUsage::None)
    {
        create.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if ((info.usage & TextureUsage::DepthStencil) != TextureUsage::None)
    {
        create.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if ((info.usage & TextureUsage::ShaderWrite) != TextureUsage::None)
    {
        create.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    // CONCURRENT across every queue family in use: no ownership transfers (ADR 0009).
    if (device.sharingFamilyCount > 1)
    {
        create.sharingMode = VK_SHARING_MODE_CONCURRENT;
        create.queueFamilyIndexCount = device.sharingFamilyCount;
        create.pQueueFamilyIndices = device.sharingFamilies;
    }
    create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // Castable formats: MUTABLE_FORMAT with the list, own format first (ADR 0022).
    if (!info.castableFormats.empty())
    {
        uint32_t count = 0;
        out.formats[count++] = create.format;
        for (PixelFormat format : info.castableFormats)
        {
            if (count < maxCastableFormats + 1)
            {
                out.formats[count++] = toVkFormat(format);
            }
        }
        create.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
        out.formatList.viewFormatCount = count;
        out.formatList.pViewFormats = out.formats;
        create.pNext = &out.formatList;
    }
}

VkImageAspectFlags aspectsOf(PixelFormat format)
{
    if (hasTraits(format, FormatTraits::Depth))
    {
        return hasTraits(format, FormatTraits::Stencil) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
                                                        : VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
    return (value + alignment - 1) / alignment * alignment;
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
    texture->aspects = aspectsOf(info.format);

    ImageInfo image;
    fillImageInfo(device, info, image);
    VkResult result = device.fn.vkCreateImage(device.device, &image.create, nullptr, &texture->image);
    if (result == VK_SUCCESS)
    {
        if (info.placement.memoryBlock)
        {
            auto* block = static_cast<MemoryBlockImpl*>(info.placement.memoryBlock);
            result = device.fn.vkBindImageMemory(device.device, texture->image, block->memory, info.placement.offset);
        }
        else
        {
            // Dedicated memory for textures too large to sub-allocate (ADR 0061).
            VkMemoryRequirements requirements = {};
            device.fn.vkGetImageMemoryRequirements(device.device, texture->image, &requirements);
            MemoryFlags flags = memoryFlags(MemoryKind::GpuOnly);
            uint32_t type = findMemoryType(device.memoryProperties, requirements.memoryTypeBits, flags.required,
                                           flags.preferred, flags.avoided);
            VkMemoryDedicatedAllocateInfo dedicated = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
            dedicated.image = texture->image;
            VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            allocateInfo.pNext = &dedicated;
            allocateInfo.allocationSize = requirements.size;
            allocateInfo.memoryTypeIndex = type;
            result = type == noMemoryType
                         ? VK_ERROR_FEATURE_NOT_PRESENT
                         : device.fn.vkAllocateMemory(device.device, &allocateInfo, nullptr, &texture->memory);
            if (result == VK_SUCCESS)
            {
                result = device.fn.vkBindImageMemory(device.device, texture->image, texture->memory, 0);
            }
        }
    }
    if (result != VK_SUCCESS)
    {
        destroyTexture(&device, texture);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(texture->image), info.debugName);
    return static_cast<TextureCommon*>(texture);
}

void destroyTexture(GraphicsDeviceCommon* common, TextureCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* texture = static_cast<TextureImpl*>(object);
    if (texture->image != VK_NULL_HANDLE)
    {
        device.fn.vkDestroyImage(device.device, texture->image, nullptr);
    }
    if (texture->memory != VK_NULL_HANDLE)
    {
        device.fn.vkFreeMemory(device.device, texture->memory, nullptr);
    }
    device.instance->allocator.destroy(texture);
}

PlacementInfo texturePlacementInfo(const GraphicsDeviceCommon* common, const TextureCreateInfo& info)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    ImageInfo image;
    fillImageInfo(device, info, image);
    VkDeviceImageMemoryRequirements query = {VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS};
    query.pCreateInfo = &image.create;
    query.planeAspect = VK_IMAGE_ASPECT_NONE;
    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
    device.fn.vkGetDeviceImageMemoryRequirements(device.device, &query, &requirements);

    // Like D3D12: 4 KB for small textures that aren't targets, 4 MB for MSAA,
    // 64 KB otherwise.
    bool target = (info.usage & (TextureUsage::RenderTarget | TextureUsage::DepthStencil)) != TextureUsage::None;
    VkDeviceSize alignment = info.sampleCount > 1                                       ? msaaAlignment
                             : !target && requirements.memoryRequirements.size <= 65536 ? smallTextureAlignment
                                                                                        : textureAlignment;
    if (requirements.memoryRequirements.alignment > alignment)
    {
        alignment = requirements.memoryRequirements.alignment;
    }
    return {alignUp(requirements.memoryRequirements.size, alignment), alignment};
}

} // namespace slag::vk
