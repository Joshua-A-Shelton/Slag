#include <iostream>
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

        void IVulkanCommandBuffer::clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            ImageBarrier barrier{.texture=texture,.oldLayout=currentLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::TRANSFER_WRITE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseArrayLayer = 0;
            range.baseMipLevel = 0;
            range.layerCount = 1;
            range.levelCount = tex->mipLevels();
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            vkCmdClearColorImage(_buffer, tex->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , reinterpret_cast<const VkClearColorValue*>(&color), 1, &range);
            barrier.oldLayout = Texture::TRANSFER_DESTINATION;
            barrier.newLayout = endingLayout;
            barrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            barrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
            barrier.syncBefore = PipelineStageFlags::TRANSFER;
            barrier.syncAfter = syncAfter;
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        }

        /*void IVulkanCommandBuffer::blit(Texture* source, Texture::Layout sourceLayout, Rectangle sourceRect, Texture* destination, Texture::Layout destinationLayout, Rectangle destinationRect, Sampler::Filter blitFilter)
        {
            for(int32_t i=0; i< destination->mipLevels(); i++)
            {
                Rectangle dstRect{.offset={destinationRect.offset.x/(i+1),destinationRect.offset.y/(i+1)},.extent={destination->width()>>i,destination->height()>>i}};
                blit(source,sourceLayout,sourceRect,0,0,destination,destinationLayout,dstRect,i,0,blitFilter);
            }
        }*/

        /*void IVulkanCommandBuffer::blit(Texture* source, Texture::Layout sourceLayout, Rectangle sourceRect, size_t sourceMipLevel, size_t sourceLayer, Texture* destination,
                                        Texture::Layout destinationLayout, Rectangle destinationRect, size_t destinationMipLevel, size_t destinationLayer, Sampler::Filter blitFilter)
        {
            auto src = dynamic_cast<VulkanTexture*>(source);
            auto dst = dynamic_cast<VulkanTexture*>(destination);
            VkImageBlit blit{};
            blit.srcOffsets[0] = {sourceRect.offset.x,sourceRect.offset.y,0};
            blit.srcOffsets[1] = {static_cast<int32_t>(sourceRect.extent.width),static_cast<int32_t>(sourceRect.extent.height),1};
            blit.srcSubresource.aspectMask = src->aspectFlags();
            blit.srcSubresource.mipLevel = sourceMipLevel;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstSubresource.aspectMask = dst->aspectFlags();
            blit.dstSubresource.mipLevel = destinationMipLevel;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(_buffer,src->image(),VulkanLib::layout(sourceLayout),dst->image(),VulkanLib::layout(destinationLayout),1,&blit,VulkanLib::filter(blitFilter));
        }*/

        void IVulkanCommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)
        {
            std::vector<VkImageMemoryBarrier2> imageMemoryBarriers(imageBarrierCount,VkImageMemoryBarrier2{});
            for(size_t i=0; i< imageBarrierCount; i++)
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
                vkbarrier.subresourceRange = {.aspectMask = texture->aspectFlags(), .baseMipLevel =barrier.baseMipLevel, .levelCount = barrier.mipCount == 0 ? texture->mipLevels()-barrier.baseMipLevel : barrier.mipCount, .baseArrayLayer = barrier.baseLayer, .layerCount = barrier.layerCount == 0 ? texture->layers()-barrier.baseLayer : barrier.layerCount};

            }
            std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers(bufferBarrierCount,VkBufferMemoryBarrier2{});
            for(size_t i=0; i< bufferBarrierCount; i++)
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
            std::vector<VkMemoryBarrier2> memBarriers(memoryBarrierCount,VkMemoryBarrier2{});
            for(size_t i=0; i< memoryBarrierCount; i++)
            {
                auto& memoryBarrier = memBarriers[i];
                auto& memoryBarrierDesc = memoryBarriers[i];
                memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                memoryBarrier.srcAccessMask = std::bit_cast<VkAccessFlags>(memoryBarrierDesc.accessBefore);
                memoryBarrier.dstAccessMask = std::bit_cast<VkAccessFlags>(memoryBarrierDesc.accessAfter);
                memoryBarrier.srcStageMask = std::bit_cast<VkPipelineStageFlags>(memoryBarrierDesc.syncBefore);
                memoryBarrier.dstStageMask = std::bit_cast<VkPipelineStageFlags>(memoryBarrierDesc.syncAfter);
            }
            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            //dependencyInfo.dependencyFlags = ;
            dependencyInfo.memoryBarrierCount = memoryBarrierCount;
            dependencyInfo.pMemoryBarriers = memBarriers.data();
            dependencyInfo.bufferMemoryBarrierCount = bufferBarrierCount;
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = imageBarrierCount;
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(_buffer,&dependencyInfo);
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


        void IVulkanCommandBuffer::copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)
        {
            auto image = dynamic_cast<VulkanTexture*>(destination);
            auto buffer = dynamic_cast<VulkanBuffer*>(source);
            uint32_t w = std::max((uint32_t)1,image->width()>>mipLevel);
            uint32_t h = std::max((uint32_t)1,image->height()>>mipLevel);

            VkBufferImageCopy copy{};
            copy.imageExtent = {.width=w,.height=h,.depth=1};
            copy.bufferOffset = sourceOffset;
            copy.imageSubresource =
                    {
                            .aspectMask = image->aspectFlags(),
                            .mipLevel = static_cast<uint32_t>(mipLevel),
                            .baseArrayLayer = static_cast<uint32_t>(layer),
                            .layerCount = 1
                    };

            vkCmdCopyBufferToImage(_buffer,buffer->underlyingBuffer(),image->image(),VulkanLib::layout(destinationLayout),1,&copy);
        }

        void IVulkanCommandBuffer::vulkanBlitImage(VkImageAspectFlags aspects, VulkanTexture* source, VkImageLayout sourceLayout, Rectangle sourceArea, uint32_t sourceLayer, uint32_t sourceMipLevel, VulkanTexture* destination, VkImageLayout destImageLayout, Rectangle destArea, uint32_t destLayer, uint32_t destMipLevel, VkFilter filter)
        {
            VkImageBlit blit{};
            blit.srcOffsets[0] = {sourceArea.offset.x,sourceArea.offset.y,0};
            blit.srcOffsets[1] = {static_cast<int32_t>(sourceArea.extent.width),static_cast<int32_t>(sourceArea.extent.height),1};
            blit.srcSubresource.aspectMask = aspects;
            blit.srcSubresource.mipLevel = sourceMipLevel;
            blit.srcSubresource.baseArrayLayer = sourceLayer;
            blit.srcSubresource.layerCount = 1;
            blit.dstSubresource.aspectMask = aspects;
            blit.dstSubresource.mipLevel = destMipLevel;
            blit.dstSubresource.baseArrayLayer = destLayer;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(_buffer,source->image(),sourceLayout,destination->image(),destImageLayout,1,&blit,filter);
        }

    } // vulkan
} // slag