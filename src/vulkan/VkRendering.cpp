#include "vulkan/VkBackend.h"

namespace slag::vk {

namespace {

GraphicsDeviceImpl& deviceOf(CommandBufferCommon* commandBuffer)
{
    return *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
}

VkCommandBuffer handle(CommandBufferCommon* commandBuffer)
{
    return static_cast<CommandBufferImpl*>(commandBuffer)->commandBuffer;
}

VkAttachmentLoadOp toLoadOp(LoadOp op)
{
    switch (op)
    {
    case LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp::Discard:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_LOAD;
}

VkAttachmentStoreOp toStoreOp(StoreOp op)
{
    // Resolve discards the MSAA samples, as D3D12's resolve ending does.
    return op == StoreOp::Store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

// Integer formats clear with integer values; ClearValue holds floats.
VkClearColorValue clearColor(PixelFormat format, const ClearValue& value)
{
    VkClearColorValue color = {};
    for (int i = 0; i < 4; ++i)
    {
        if (hasTraits(format, FormatTraits::Integer))
        {
            color.int32[i] = static_cast<int32_t>(value.color[i]); // the bit pattern serves uint formats too
        }
        else
        {
            color.float32[i] = value.color[i];
        }
    }
    return color;
}

// Origin top-left with +Y up in clip space on both backends: Vulkan's
// viewport is flipped (negative height, core since 1.1).
VkViewport flipped(const Viewport& viewport)
{
    return {viewport.x,       viewport.y + viewport.height, viewport.width, -viewport.height, viewport.minDepth,
            viewport.maxDepth};
}

} // namespace

void cmdBeginRendering(CommandBufferCommon* object, const RenderingDesc& desc, uint32_t width, uint32_t height,
                       uint32_t layerCount)
{
    GraphicsDeviceImpl& device = deviceOf(object);
    VkRenderingAttachmentInfo colors[8] = {};
    uint32_t colorCount = static_cast<uint32_t>(desc.colorAttachments.size() < 8 ? desc.colorAttachments.size() : 8);
    for (uint32_t i = 0; i < colorCount; ++i)
    {
        const ColorAttachment& attachment = desc.colorAttachments[i];
        VkRenderingAttachmentInfo& color = colors[i];
        color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        auto* view = static_cast<AttachmentViewImpl*>(attachment.view);
        if (!view)
        {
            continue; // a gap: VK_NULL_HANDLE
        }
        color.imageView = view->view;
        color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color.loadOp = toLoadOp(attachment.loadOp);
        color.storeOp = toStoreOp(attachment.storeOp);
        color.clearValue.color = clearColor(view->format, attachment.clearValue);
        if (attachment.storeOp == StoreOp::Resolve && attachment.resolveView)
        {
            color.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
            color.resolveImageView = static_cast<AttachmentViewImpl*>(attachment.resolveView)->view;
            color.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }

    VkRenderingAttachmentInfo depth = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    VkRenderingAttachmentInfo stencil = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    bool hasDepth = false;
    bool hasStencil = false;
    if (desc.depthStencilAttachment && desc.depthStencilAttachment->view)
    {
        const DepthStencilAttachment& attachment = *desc.depthStencilAttachment;
        auto* view = static_cast<AttachmentViewImpl*>(attachment.view);
        hasDepth = true;
        hasStencil = hasTraits(view->format, FormatTraits::Stencil);
        bool readOnlyStencil = !hasStencil || attachment.readOnlyStencil;
        VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if (attachment.readOnlyDepth && readOnlyStencil)
        {
            layout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL; // DepthStencilRead (ADR 0049)
        }
        else if (attachment.readOnlyDepth)
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
        }
        else if (hasStencil && attachment.readOnlyStencil)
        {
            layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
        }
        depth.imageView = view->view;
        depth.imageLayout = layout;
        depth.loadOp = toLoadOp(attachment.depthLoadOp);
        depth.storeOp = toStoreOp(attachment.depthStoreOp);
        depth.clearValue.depthStencil = {attachment.clearValue.depth, attachment.clearValue.stencil};
        stencil = depth;
        stencil.loadOp = toLoadOp(attachment.stencilLoadOp);
        stencil.storeOp = toStoreOp(attachment.stencilStoreOp);
    }

    VkRenderingInfo info = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.flags = (desc.suspending ? VK_RENDERING_SUSPENDING_BIT : 0) | (desc.resuming ? VK_RENDERING_RESUMING_BIT : 0);
    info.renderArea = {{0, 0}, {width, height}};
    info.layerCount = layerCount;
    info.colorAttachmentCount = colorCount;
    info.pColorAttachments = colors;
    info.pDepthAttachment = hasDepth ? &depth : nullptr;
    info.pStencilAttachment = hasStencil ? &stencil : nullptr;
    device.fn.vkCmdBeginRendering(handle(object), &info);

    // The full render area to start with (CommandBuffer::beginRendering).
    VkViewport viewport = flipped({0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f});
    VkRect2D scissor = {{0, 0}, {width, height}};
    device.fn.vkCmdSetViewportWithCount(handle(object), 1, &viewport);
    device.fn.vkCmdSetScissorWithCount(handle(object), 1, &scissor);
}

void cmdEndRendering(CommandBufferCommon* object)
{
    deviceOf(object).fn.vkCmdEndRendering(handle(object));
}

void cmdBindVertexBuffers(CommandBufferCommon* object, uint32_t firstSlot,
                          std::span<const VertexBufferBinding> bindings)
{
    VkBuffer buffers[32];
    VkDeviceSize offsets[32];
    VkDeviceSize sizes[32];
    VkDeviceSize strides[32];
    uint32_t count = static_cast<uint32_t>(bindings.size() < 32 ? bindings.size() : 32);
    for (uint32_t i = 0; i < count; ++i)
    {
        buffers[i] = static_cast<BufferImpl*>(bindings[i].buffer)->buffer;
        offsets[i] = bindings[i].offset;
        sizes[i] = bindings[i].size;
        strides[i] = bindings[i].stride; // dynamic stride (ADR 0038)
    }
    deviceOf(object).fn.vkCmdBindVertexBuffers2(handle(object), firstSlot, count, buffers, offsets, sizes, strides);
}

void cmdBindIndexBuffer(CommandBufferCommon* object, BufferCommon* buffer, uint64_t offset, uint64_t size,
                        IndexFormat format)
{
    deviceOf(object).fn.vkCmdBindIndexBuffer2(handle(object), static_cast<BufferImpl*>(buffer)->buffer, offset, size,
                                              format == IndexFormat::Uint16 ? VK_INDEX_TYPE_UINT16
                                                                            : VK_INDEX_TYPE_UINT32);
}

void cmdSetViewports(CommandBufferCommon* object, std::span<const Viewport> viewports)
{
    VkViewport converted[16];
    uint32_t count = static_cast<uint32_t>(viewports.size() < 16 ? viewports.size() : 16);
    for (uint32_t i = 0; i < count; ++i)
    {
        converted[i] = flipped(viewports[i]);
    }
    deviceOf(object).fn.vkCmdSetViewportWithCount(handle(object), count, converted);
}

void cmdSetScissors(CommandBufferCommon* object, std::span<const Rect> scissors)
{
    VkRect2D converted[16];
    uint32_t count = static_cast<uint32_t>(scissors.size() < 16 ? scissors.size() : 16);
    for (uint32_t i = 0; i < count; ++i)
    {
        converted[i] = {{scissors[i].x, scissors[i].y}, {scissors[i].width, scissors[i].height}};
    }
    deviceOf(object).fn.vkCmdSetScissorWithCount(handle(object), count, converted);
}

void cmdSetBlendConstant(CommandBufferCommon* object, const float constant[4])
{
    deviceOf(object).fn.vkCmdSetBlendConstants(handle(object), constant);
}

void cmdSetStencilReference(CommandBufferCommon* object, uint8_t reference)
{
    deviceOf(object).fn.vkCmdSetStencilReference(handle(object), VK_STENCIL_FACE_FRONT_AND_BACK, reference);
}

void cmdSetDepthBounds(CommandBufferCommon* object, float minDepth, float maxDepth)
{
    deviceOf(object).fn.vkCmdSetDepthBounds(handle(object), minDepth, maxDepth);
}

void cmdDraw(CommandBufferCommon* object, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
             uint32_t firstInstance)
{
    deviceOf(object).fn.vkCmdDraw(handle(object), vertexCount, instanceCount, firstVertex, firstInstance);
}

void cmdDrawIndexed(CommandBufferCommon* object, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                    int32_t vertexOffset, uint32_t firstInstance)
{
    deviceOf(object).fn.vkCmdDrawIndexed(handle(object), indexCount, instanceCount, firstIndex, vertexOffset,
                                         firstInstance);
}

} // namespace slag::vk
