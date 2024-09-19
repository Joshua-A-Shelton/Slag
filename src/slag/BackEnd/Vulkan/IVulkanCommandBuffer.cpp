#include "IVulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        void IVulkanCommandBuffer::move(IVulkanCommandBuffer& from)
        {
            std::swap(_buffer,from._buffer);
            std::swap(_pool,from._pool);
            _family = from._family;
        }

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

        void IVulkanCommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)
        {
            std::vector<VkImageMemoryBarrier2> imageMemoryBarriers(imageBarrierCount,VkImageMemoryBarrier2{});
            for(int i=0; i< imageBarrierCount; i++)
            {
                auto& vkbarrier = imageMemoryBarriers[i];
                auto barrier = imageBarriers[i];
                auto texture = dynamic_cast<VulkanTexture*>(barrier.texture);
                vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                vkbarrier.srcAccessMask = std::bit_cast<VkAccessFlags>(barrier.accessBefore);
                vkbarrier.dstAccessMask = std::bit_cast<VkAccessFlags>(barrier.accessAfter);
                vkbarrier.image = texture->image();
                vkbarrier.oldLayout = VulkanLib::layout(barrier.oldLayout);
                vkbarrier.newLayout = VulkanLib::layout(barrier.newLayout);
                vkbarrier.srcStageMask = std::bit_cast<VkPipelineStageFlags>(barrier.syncBefore);
                vkbarrier.dstStageMask = std::bit_cast<VkPipelineStageFlags>(barrier.syncAfter);
                vkbarrier.subresourceRange = {.aspectMask = texture->aspectFlags(), .baseMipLevel = 0, .levelCount = texture->mipLevels(), .baseArrayLayer = 0, .layerCount = 1};

            }
            std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers(bufferBarrierCount,VkBufferMemoryBarrier2{});
            for(auto i=0; i< bufferBarrierCount; i++)
            {
                auto& bufferBarrier = bufferMemoryBarriers[i];
                auto bufferBarrierDesc = bufferBarriers[i];
                auto buffer = dynamic_cast<VulkanBuffer*>(bufferBarrierDesc.buffer);
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                bufferBarrier.buffer = buffer->underlyingBuffer();
                bufferBarrier.srcAccessMask = std::bit_cast<VkAccessFlags>(bufferBarrierDesc.accessBefore);
                bufferBarrier.dstAccessMask = std::bit_cast<VkAccessFlags>(bufferBarrierDesc.accessAfter);
                bufferBarrier.offset = bufferBarrierDesc.offset;
                bufferBarrier.size = bufferBarrierDesc.size != 0 ? bufferBarrierDesc.size : VK_WHOLE_SIZE;
                bufferBarrier.srcStageMask = std::bit_cast<VkPipelineStageFlags>(bufferBarrierDesc.syncBefore);
                bufferBarrier.dstStageMask = std::bit_cast<VkPipelineStageFlags>(bufferBarrierDesc.syncAfter);
            }
            throw std::runtime_error("finish pipeline stage implementation for barriers");
            /*vkCmdPipelineBarrier2();
            vkCmdPipelineBarrier(_buffer,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,bufferBarrierCount,bufferMemoryBarriers.data(),imageBarrierCount,imageMemoryBarriers.data());*/
        }

        void IVulkanCommandBuffer::copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)
        {
            VulkanBuffer* src = dynamic_cast<VulkanBuffer*>(source);
            VulkanBuffer* dst = dynamic_cast<VulkanBuffer*>(destination);

            VkBufferCopy copyRegion = {};
            copyRegion.size = length;
            copyRegion.srcOffset = sourceOffset;
            copyRegion.dstOffset = destinationOffset;

            //copy the buffer into the image
            vkCmdCopyBuffer(_buffer, src->underlyingBuffer(), dst->underlyingBuffer(), 1, &copyRegion);
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

        void IVulkanCommandBuffer::copyBufferToImageMip(VulkanBuffer* buffer, VkDeviceSize bufferOffset, VulkanTexture* image, uint32_t mipLevel, VkImageLayout destinationImageLayout)
        {

            uint32_t w = std::max((uint32_t)1,image->width()>>mipLevel);
            uint32_t h = std::max((uint32_t)1,image->height()>>mipLevel);

            VkBufferImageCopy copy{};
            copy.imageExtent = {.width=w,.height=h,.depth=1};
            copy.bufferOffset = bufferOffset;
            copy.imageSubresource =
            {
                .aspectMask = image->aspectFlags(),
                .mipLevel = mipLevel,
                .baseArrayLayer = 0,
                .layerCount = 1
            };

            vkCmdCopyBufferToImage(_buffer,buffer->underlyingBuffer(),image->image(),destinationImageLayout,1,&copy);
        }
    } // vulkan
} // slag