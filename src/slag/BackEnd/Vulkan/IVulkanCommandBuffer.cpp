#include "IVulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"

namespace slag
{
    namespace vulkan
    {
        void IVulkanCommandBuffer::end()
        {
            vkEndCommandBuffer(_buffer);
        }
        GpuQueue::QueueType IVulkanCommandBuffer::commandType()
        {
            if(_family == VulkanLib::card()->graphicsQueueFamily())
            {
                return GpuQueue::Graphics;
            }
            else if(_family == VulkanLib::card()->computeQueueFamily())
            {
                return GpuQueue::Compute;
            }
            else
            {
                return GpuQueue::Transfer;
            }
        }

        VkCommandBuffer IVulkanCommandBuffer::underlyingCommandBuffer()
        {
            return _buffer;
        }

        void IVulkanCommandBuffer::clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)
        {
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseArrayLayer = 0;
            range.baseMipLevel = 0;
            range.layerCount = 1;
            range.levelCount = tex->mipLevels();
            vkCmdClearColorImage(_buffer, tex->image(), VulkanLib::layout(layout),
                                 reinterpret_cast<const VkClearColorValue*>(&color), 1, &range);
        }

        void IVulkanCommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount,BufferBarrier* bufferBarriers, size_t bufferBarrierCount)
        {
            std::vector<VkImageMemoryBarrier> imageMemoryBarriers(imageBarrierCount,VkImageMemoryBarrier{});
            for(int i=0; i< imageBarrierCount; i++)
            {
                auto& vkbarrier = imageMemoryBarriers[i];
                auto barrier = imageBarriers[i];
                auto texture = dynamic_cast<VulkanTexture*>(barrier.texture);
                vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                vkbarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                vkbarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
                vkbarrier.image = texture->image();
                vkbarrier.oldLayout = VulkanLib::layout(barrier.oldLayout);
                vkbarrier.newLayout = VulkanLib::layout(barrier.newLayout);
                vkbarrier.subresourceRange = {.aspectMask = texture->aspectFlags(), .baseMipLevel = 0, .levelCount = texture->mipLevels(), .baseArrayLayer = 0, .layerCount = 1};

            }
            std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers(bufferBarrierCount,VkBufferMemoryBarrier{});
            vkCmdPipelineBarrier(_buffer,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,bufferBarrierCount,bufferMemoryBarriers.data(),imageBarrierCount,imageMemoryBarriers.data());
        }

        void IVulkanCommandBuffer::transitionImageSubResource(VulkanTexture* texture, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t startingMipLevel, uint32_t levelCount, uint32_t startingLayer, uint32_t layerCount)
        {
            VkImageMemoryBarrier vkbarrier{};
            vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            vkbarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkbarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkbarrier.image = texture->image();
            vkbarrier.oldLayout = oldLayout;
            vkbarrier.newLayout = newLayout;
            vkbarrier.subresourceRange = {.aspectMask = texture->aspectFlags(), .baseMipLevel = startingMipLevel, .levelCount = levelCount, .baseArrayLayer = startingLayer, .layerCount = layerCount};
            vkCmdPipelineBarrier(_buffer,srcStageMask,dstStageMask,0,0, nullptr,0, nullptr,1,&vkbarrier);
        }

        void IVulkanCommandBuffer::blitSubResource(VkImageAspectFlags aspects, VulkanTexture* source, VkImageLayout sourceLayout, Rectangle sourceArea, uint32_t sourceMipLevel, VulkanTexture* destination, VkImageLayout destImageLayout, Rectangle destArea, uint32_t  destMipLevel, VkFilter filter)
        {
            VkImageBlit blit{};
            blit.srcOffsets[0] = {sourceArea.offset.x,sourceArea.offset.y,0};
            blit.srcOffsets[1] = {static_cast<int32_t>(sourceArea.extent.width),static_cast<int32_t>(sourceArea.extent.height),1};
            blit.srcSubresource.aspectMask = aspects;
            blit.srcSubresource.mipLevel = sourceMipLevel;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstSubresource.aspectMask = aspects;
            blit.dstSubresource.mipLevel = destMipLevel;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(_buffer,source->image(),sourceLayout,destination->image(),destImageLayout,1,&blit,filter);
        }
    } // vulkan
} // slag