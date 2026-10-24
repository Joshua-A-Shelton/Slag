#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"

namespace slag::vk {

namespace {

GraphicsDeviceImpl& deviceOf(GraphicsDeviceCommon* common)
{
    return *static_cast<GraphicsDeviceImpl*>(common);
}

VkHostAddressRangeEXT slot(const DescriptorHeap& heap, uint32_t index, VkDeviceSize size)
{
    // Slots are the largest descriptor size apart (ADR 0029).
    return {heap.mapped + uint64_t(index) * heap.stride, static_cast<size_t>(size)};
}

VkImageViewType viewType(TextureViewType type)
{
    switch (type)
    {
    case TextureViewType::Texture1D:
        return VK_IMAGE_VIEW_TYPE_1D;
    case TextureViewType::Texture1DArray:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case TextureViewType::Texture2D:
    case TextureViewType::Texture2DMS:
        return VK_IMAGE_VIEW_TYPE_2D;
    case TextureViewType::Texture2DArray:
    case TextureViewType::Texture2DMSArray:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case TextureViewType::Texture3D:
        return VK_IMAGE_VIEW_TYPE_3D;
    case TextureViewType::TextureCube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case TextureViewType::TextureCubeArray:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }
    return VK_IMAGE_VIEW_TYPE_2D;
}

VkFilter toFilter(Filter filter)
{
    return filter == Filter::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
}

VkSamplerAddressMode toAddressMode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::Wrap:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::Mirror:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::Clamp:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::Border:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case AddressMode::MirrorOnce:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkBorderColor toBorderColor(BorderColor color)
{
    switch (color)
    {
    case BorderColor::TransparentBlack:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case BorderColor::OpaqueBlack:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case BorderColor::OpaqueWhite:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    }
    return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
}

VkSamplerReductionMode toReductionMode(ReductionMode mode)
{
    switch (mode)
    {
    case ReductionMode::Standard:
        return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
    case ReductionMode::Minimum:
        return VK_SAMPLER_REDUCTION_MODE_MIN;
    case ReductionMode::Maximum:
        return VK_SAMPLER_REDUCTION_MODE_MAX;
    }
    return VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
}

} // namespace

VkCompareOp toCompareOp(CompareOp op)
{
    static_assert(static_cast<int>(CompareOp::Never) == VK_COMPARE_OP_NEVER &&
                  static_cast<int>(CompareOp::LessEqual) == VK_COMPARE_OP_LESS_OR_EQUAL &&
                  static_cast<int>(CompareOp::Always) == VK_COMPARE_OP_ALWAYS);
    return static_cast<VkCompareOp>(op); // same order
}

void writeBufferDescriptor(GraphicsDeviceCommon* common, uint32_t index, const BufferDescriptorInfo& info)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    VkResourceDescriptorInfoEXT resource = {VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT};
    VkDeviceAddressRangeEXT range = {info.address, info.size};
    VkTexelBufferDescriptorInfoEXT texel = {VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT};
    VkDeviceSize size = device.heapProperties.bufferDescriptorSize;
    switch (info.kind)
    {
    case BufferDescriptorKind::Constant:
        resource.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        resource.data.pAddressRange = &range;
        break;
    case BufferDescriptorKind::Raw:
    case BufferDescriptorKind::Structured:
    case BufferDescriptorKind::RWRaw:
    case BufferDescriptorKind::RWStructured:
        resource.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        resource.data.pAddressRange = &range;
        break;
    case BufferDescriptorKind::Typed:
    case BufferDescriptorKind::RWTyped:
        resource.type = info.kind == BufferDescriptorKind::Typed ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                                                                 : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        texel.format = toVkFormat(info.format);
        texel.addressRange = range;
        resource.data.pTexelBuffer = &texel;
        size = device.heapProperties.imageDescriptorSize; // texel buffers are image-sized
        break;
    }
    VkHostAddressRangeEXT destination = slot(device.resourceHeap, index, size);
    device.fn.vkWriteResourceDescriptorsEXT(device.device, 1, &resource, &destination);
}

void writeTextureDescriptor(GraphicsDeviceCommon* common, uint32_t index, const TextureDescriptorInfo& info)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    auto* texture = static_cast<TextureImpl*>(info.texture);
    VkImageViewCreateInfo view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = texture->image;
    view.viewType = viewType(info.viewType);
    view.format = toVkFormat(info.format);
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    if (hasTraits(texture->format, FormatTraits::Depth))
    {
        aspect = info.plane == TexturePlane::Stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    view.subresourceRange = {aspect, info.baseMip, info.mipCount, info.baseLayer, info.layerCount};
    if (info.viewType == TextureViewType::Texture3D)
    {
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;
    }

    // Sampled textures are in ShaderRead or DepthStencilRead, both
    // READ_ONLY_OPTIMAL; written ones in ShaderWrite (GENERAL) (ADR 0049).
    VkImageDescriptorInfoEXT image = {VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT};
    image.pView = &view;
    image.layout = info.writable ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
    VkResourceDescriptorInfoEXT resource = {VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT};
    resource.type = info.writable ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    resource.data.pImage = &image;
    VkHostAddressRangeEXT destination = slot(device.resourceHeap, index, device.heapProperties.imageDescriptorSize);
    device.fn.vkWriteResourceDescriptorsEXT(device.device, 1, &resource, &destination);
}

void writeSamplerDescriptor(GraphicsDeviceCommon* common, uint32_t index, const SamplerDescriptor& descriptor)
{
    GraphicsDeviceImpl& device = deviceOf(common);
    VkSamplerReductionModeCreateInfo reduction = {VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO};
    reduction.reductionMode = toReductionMode(descriptor.reductionMode);
    VkSamplerCreateInfo sampler = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    if (descriptor.reductionMode != ReductionMode::Standard)
    {
        sampler.pNext = &reduction;
    }
    sampler.magFilter = toFilter(descriptor.magFilter);
    sampler.minFilter = toFilter(descriptor.minFilter);
    sampler.mipmapMode =
        descriptor.mipFilter == Filter::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = toAddressMode(descriptor.addressU);
    sampler.addressModeV = toAddressMode(descriptor.addressV);
    sampler.addressModeW = toAddressMode(descriptor.addressW);
    sampler.mipLodBias = descriptor.mipLodBias;
    sampler.anisotropyEnable = descriptor.maxAnisotropy > 1 ? VK_TRUE : VK_FALSE;
    sampler.maxAnisotropy = static_cast<float>(descriptor.maxAnisotropy);
    sampler.compareEnable = descriptor.compareEnable ? VK_TRUE : VK_FALSE;
    sampler.compareOp = toCompareOp(descriptor.compareOp);
    sampler.minLod = descriptor.minLod;
    sampler.maxLod = descriptor.maxLod;
    sampler.borderColor = toBorderColor(descriptor.borderColor);
    VkHostAddressRangeEXT destination = slot(device.samplerHeap, index, device.heapProperties.samplerDescriptorSize);
    device.fn.vkWriteSamplerDescriptorsEXT(device.device, 1, &sampler, &destination);
}

} // namespace slag::vk
