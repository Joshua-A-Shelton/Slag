#include "vulkan/VkBackend.h"

#include <utility>

namespace slag::vk {

void swapBufferNative(BufferCommon* a, BufferCommon* b)
{
    auto* first = static_cast<BufferImpl*>(a);
    auto* second = static_cast<BufferImpl*>(b);
    std::swap(first->buffer, second->buffer);
    std::swap(first->memory, second->memory);
    std::swap(first->mapped, second->mapped);
    std::swap(first->gpuAddress, second->gpuAddress);
}

void swapTextureNative(TextureCommon* a, TextureCommon* b)
{
    auto* first = static_cast<TextureImpl*>(a);
    auto* second = static_cast<TextureImpl*>(b);
    std::swap(first->image, second->image);
    std::swap(first->memory, second->memory);
}

void swapAttachmentViewNative(AttachmentViewCommon* a, AttachmentViewCommon* b)
{
    std::swap(static_cast<AttachmentViewImpl*>(a)->view, static_cast<AttachmentViewImpl*>(b)->view);
}

// New images start UNDEFINED. On graphics and compute queues they go to
// TRANSFER_DST for the copy and end in TRANSFER_SRC, the layout the old ones
// are in (CopySource); copy queues use GENERAL (Common) throughout. Transfer
// queues may change image layouts in Vulkan (ADR 0062).
void cmdRecordMoves(CommandBufferCommon* object, const MoveEntry* moves, size_t count)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
    bool copyQueue = commandBuffer->queueType == CommandQueueType::Copy;
    VkImageLayout copyLayout = copyQueue ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    VkImageLayout sourceLayout = copyQueue ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    Vector<VkImageMemoryBarrier2>& barriers = commandBuffer->imageBarriers;
    barriers.clear();
    for (size_t i = 0; i < count; ++i)
    {
        if (!moves[i].texture)
        {
            continue;
        }
        auto* texture = static_cast<TextureImpl*>(moves[i].newTexture);
        VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = copyLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture->image;
        barrier.subresourceRange = {texture->aspects, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};
        (void)barriers.pushBack(barrier); // keeps its capacity across reset()
    }
    VkDependencyInfo dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    if (!barriers.empty())
    {
        dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
        dependency.pImageMemoryBarriers = barriers.data();
        device.fn.vkCmdPipelineBarrier2(commandBuffer->commandBuffer, &dependency);
    }

    for (size_t i = 0; i < count; ++i)
    {
        const MoveEntry& move = moves[i];
        if (move.buffer)
        {
            VkBufferCopy region = {0, 0, move.buffer->size};
            device.fn.vkCmdCopyBuffer(commandBuffer->commandBuffer, static_cast<BufferImpl*>(move.buffer)->buffer,
                                      static_cast<BufferImpl*>(move.newBuffer)->buffer, 1, &region);
        }
        else if (move.texture)
        {
            const TextureImpl& source = *static_cast<TextureImpl*>(move.texture);
            VkImageCopy2 regions[16] = {};
            uint32_t layers = source.type == TextureType::Texture3D ? 1 : source.layerCount;
            for (uint32_t mip = 0; mip < source.mipCount && mip < 16; ++mip)
            {
                VkImageCopy2& region = regions[mip];
                region.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
                region.srcSubresource = {source.aspects, mip, 0, layers};
                region.dstSubresource = region.srcSubresource;
                region.extent = {source.width >> mip ? source.width >> mip : 1,
                                 source.height >> mip ? source.height >> mip : 1,
                                 source.depth >> mip ? source.depth >> mip : 1};
            }
            VkCopyImageInfo2 info = {VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2};
            info.srcImage = source.image;
            info.srcImageLayout = sourceLayout;
            info.dstImage = static_cast<TextureImpl*>(move.newTexture)->image;
            info.dstImageLayout = copyLayout;
            info.regionCount = source.mipCount < 16 ? source.mipCount : 16;
            info.pRegions = regions;
            device.fn.vkCmdCopyImage2(commandBuffer->commandBuffer, &info);
        }
    }

    // The copies finish before anything else touches the new resources.
    VkMemoryBarrier2 memory = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    memory.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    memory.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &memory;
    if (!copyQueue)
    {
        for (VkImageMemoryBarrier2& barrier : barriers)
        {
            barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
            barrier.dstAccessMask = VK_ACCESS_2_NONE;
            barrier.oldLayout = copyLayout;
            barrier.newLayout = sourceLayout;
        }
        dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
        dependency.pImageMemoryBarriers = barriers.data();
    }
    device.fn.vkCmdPipelineBarrier2(commandBuffer->commandBuffer, &dependency);
}

} // namespace slag::vk
