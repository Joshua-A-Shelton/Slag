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

// Every earlier use of a moved resource is in an earlier submit, ordered by
// the submit's boundary barrier (ADR 0049); the old image's barrier starts at
// the copy stage so it chains to that barrier. On graphics and compute queues
// the old image goes from its layout to TRANSFER_SRC and the new one from
// UNDEFINED to TRANSFER_DST, then to the old one's layout; copy queues use
// GENERAL (Common) throughout. Textures passed as Undefined aren't copied
// (ADR 0062).
void cmdRecordMoves(CommandBufferCommon* object, const MoveEntry* moves, size_t count)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
    bool copyQueue = commandBuffer->queueType == CommandQueueType::Copy;
    VkImageLayout copyLayout = copyQueue ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    VkImageLayout sourceLayout = copyQueue ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    Vector<VkImageMemoryBarrier2>& barriers = commandBuffer->imageBarriers;
    barriers.clear();
    auto imageBarrier = [](const TextureImpl& texture) {
        VkImageMemoryBarrier2 barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = texture.image;
        barrier.subresourceRange = {texture.aspects, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS};
        return barrier;
    };
    for (size_t i = 0; i < count; ++i)
    {
        const MoveEntry& move = moves[i];
        if (!move.texture || move.layout == TextureLayout::Undefined)
        {
            continue;
        }
        if (!copyQueue && move.layout != TextureLayout::CopySource)
        {
            VkImageMemoryBarrier2 old = imageBarrier(*static_cast<TextureImpl*>(move.texture));
            old.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            old.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            old.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            old.oldLayout = imageLayout(move.layout);
            old.newLayout = sourceLayout;
            (void)barriers.pushBack(old); // keeps its capacity across reset()
        }
        VkImageMemoryBarrier2 created = imageBarrier(*static_cast<TextureImpl*>(move.newTexture));
        created.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
        created.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        created.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        created.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        created.newLayout = copyLayout;
        (void)barriers.pushBack(created);
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
        else if (move.texture && move.layout != TextureLayout::Undefined)
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

    // The copies finish before anything else touches the new resources; new
    // images end in the old ones' layouts.
    barriers.clear();
    for (size_t i = 0; i < count && !copyQueue; ++i)
    {
        const MoveEntry& move = moves[i];
        if (move.texture && move.layout != TextureLayout::Undefined)
        {
            VkImageMemoryBarrier2 done = imageBarrier(*static_cast<TextureImpl*>(move.newTexture));
            done.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
            done.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            done.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
            done.oldLayout = copyLayout;
            done.newLayout = imageLayout(move.layout);
            (void)barriers.pushBack(done);
        }
    }
    VkMemoryBarrier2 memory = {VK_STRUCTURE_TYPE_MEMORY_BARRIER_2};
    memory.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    memory.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    memory.dstStageMask = VK_PIPELINE_STAGE_2_NONE;
    dependency = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dependency.memoryBarrierCount = 1;
    dependency.pMemoryBarriers = &memory;
    dependency.imageMemoryBarrierCount = static_cast<uint32_t>(barriers.size());
    dependency.pImageMemoryBarriers = barriers.data();
    device.fn.vkCmdPipelineBarrier2(commandBuffer->commandBuffer, &dependency);
}

} // namespace slag::vk
