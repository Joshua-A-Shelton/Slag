#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"

namespace slag::vk {

namespace {

// Derived from the texture's type and the layer count (ADR 0035). Cube faces
// and 3D slices are rendered as 2D array layers.
VkImageViewType viewType(TextureType type, uint32_t layerCount)
{
    switch (type)
    {
    case TextureType::Texture1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TextureType::Texture1DArray:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case TextureType::Texture2D:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureType::Texture2DArray:
    case TextureType::Texture3D:
    case TextureType::TextureCube:
    case TextureType::TextureCubeArray:
        return layerCount == 1 && type != TextureType::Texture2DArray ? VK_IMAGE_VIEW_TYPE_2D
                                                                      : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    return VK_IMAGE_VIEW_TYPE_2D;
}

} // namespace

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

    VkImageViewCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    info.image = texture->image;
    info.viewType = viewType(texture->type, desc.layerCount);
    info.format = toVkFormat(desc.format);
    info.subresourceRange = {texture->aspects, desc.mip, 1, desc.baseLayer, desc.layerCount};
    VkResult result = device.fn.vkCreateImageView(device.device, &info, nullptr, &view->view);
    if (result != VK_SUCCESS)
    {
        device.instance->allocator.destroy(view);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, reinterpret_cast<uint64_t>(view->view), desc.debugName);
    return static_cast<AttachmentViewCommon*>(view);
}

void destroyAttachmentView(GraphicsDeviceCommon* common, AttachmentViewCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* view = static_cast<AttachmentViewImpl*>(object);
    device.fn.vkDestroyImageView(device.device, view->view, nullptr);
    device.instance->allocator.destroy(view);
}

} // namespace slag::vk
