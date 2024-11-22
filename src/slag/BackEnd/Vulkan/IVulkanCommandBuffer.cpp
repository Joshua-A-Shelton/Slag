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
            assert(commandType() == GpuQueue::Graphics && "clearColorImage is a graphics queue only operation");
            ImageBarrier barrier{.texture=texture,.oldLayout=currentLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::TRANSFER_WRITE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseArrayLayer = 0;
            range.baseMipLevel = 0;
            range.layerCount = 1;
            range.levelCount = tex->mipLevels();
            vkCmdClearColorImage(_buffer, tex->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , reinterpret_cast<const VkClearColorValue*>(&color), 1, &range);
            barrier.oldLayout = Texture::TRANSFER_DESTINATION;
            barrier.newLayout = endingLayout;
            barrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            barrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
            barrier.syncBefore = PipelineStageFlags::TRANSFER;
            barrier.syncAfter = syncAfter;
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        }

        void IVulkanCommandBuffer::updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            assert(commandType() == GpuQueue::Graphics && "clearColorImage is a graphics queue only operation");
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            ImageBarrier barriers[2];
            barriers[0]={.texture=texture,.baseLayer=0,.layerCount=0,.baseMipLevel=sourceMipLevel,.mipCount=1,.oldLayout=sourceLayout,.newLayout=Texture::TRANSFER_SOURCE,.accessBefore=BarrierAccessFlags::ALL_WRITE | BarrierAccessFlags::TRANSFER_READ,.accessAfter=BarrierAccessFlags::NONE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            barriers[1]={.texture=texture,.baseLayer=0,.layerCount=0,.baseMipLevel=sourceMipLevel+1,.mipCount=0,.oldLayout=destinationLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::NONE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            insertBarriers(barriers,2, nullptr,0, nullptr,0);
            int32_t width = tex->width() >> sourceMipLevel;
            int32_t height = tex->height() >> sourceMipLevel;
            int32_t destWidth = width;
            int32_t destHeight = height;
            for(uint32_t i=sourceMipLevel+1; i< tex->mipLevels(); i++)
            {
                destWidth = destWidth >> 1;
                destHeight = destHeight >> 1;
                VkImageBlit blitRegion
                {
                    .srcSubresource = {.aspectMask = tex->aspectFlags(),.mipLevel=sourceMipLevel,.baseArrayLayer = 0,.layerCount=tex->layers()},
                    .srcOffsets = {{0,0,0},{width,height,1}},
                    .dstSubresource = {.aspectMask = tex->aspectFlags(),.mipLevel=i,.baseArrayLayer = 0,.layerCount=tex->layers()},
                    .dstOffsets = {{0,0,0},{destWidth,destHeight,1}}

                };
                vkCmdBlitImage(_buffer,tex->image(),VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,tex->image(),VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&blitRegion,VK_FILTER_NEAREST);
            }
            auto& sourceBarrier = barriers[0];
            auto& destBarrier = barriers[1];

            sourceBarrier.oldLayout = Texture::TRANSFER_SOURCE;
            sourceBarrier.newLayout = endingSourceLayout;
            sourceBarrier.accessBefore = BarrierAccessFlags::TRANSFER_READ;
            sourceBarrier.accessAfter = BarrierAccessFlags::NONE;
            sourceBarrier.syncBefore = PipelineStageFlags::TRANSFER;
            sourceBarrier.syncAfter = syncAfter;

            destBarrier.oldLayout = Texture::TRANSFER_DESTINATION;
            destBarrier.newLayout = endingDestinationLayout;
            destBarrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            destBarrier.accessAfter = BarrierAccessFlags::ALL_READ;
            destBarrier.syncBefore = PipelineStageFlags::TRANSFER;
            destBarrier.syncAfter = syncAfter;
            insertBarriers(barriers,2, nullptr,0, nullptr,0);
        }

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

        void IVulkanCommandBuffer::copyImageToBuffer(Texture* texture, Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip, Buffer* buffer,size_t bufferOffset)
        {
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            auto buf = dynamic_cast<VulkanBuffer*>(buffer);
            VkBufferImageCopy copyRegion
            {
                .bufferOffset = bufferOffset,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = {.aspectMask=tex->aspectFlags(),.mipLevel=mip,.baseArrayLayer=baseLayer,.layerCount=layerCount},
                .imageOffset = {0,0,0},
                .imageExtent = {tex->width()>>mip,tex->height()>>mip,1}
            };
            vkCmdCopyImageToBuffer(_buffer,tex->image(),VulkanLib::layout(layout),buf->underlyingBuffer(),1,&copyRegion);
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


        void IVulkanCommandBuffer::blit(Texture* source, Texture::Layout sourceLayout, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination,Texture::Layout destinationLayout, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea, Sampler::Filter filter)
        {
            assert(commandType() == GpuQueue::Graphics && "clearColorImage is a graphics queue only operation");
            auto src = dynamic_cast<VulkanTexture*>(source);
            auto dst = dynamic_cast<VulkanTexture*>(destination);
            VkImageBlit blit{};
            blit.srcOffsets[0] = {sourceArea.offset.x,sourceArea.offset.y,0};
            blit.srcOffsets[1] = {static_cast<int32_t>(sourceArea.extent.width + sourceArea.offset.x),static_cast<int32_t>(sourceArea.extent.height + sourceArea.offset.y),1};
            blit.srcSubresource.aspectMask = src->aspectFlags();
            blit.srcSubresource.mipLevel = sourceMip;
            blit.srcSubresource.baseArrayLayer = sourceLayer;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {destinationArea.offset.x,destinationArea.offset.y,0};
            blit.dstOffsets[1] = {static_cast<int32_t>(destinationArea.extent.width + destinationArea.offset.x),static_cast<int32_t>(destinationArea.extent.height + destinationArea.offset.y),1};
            blit.dstSubresource.aspectMask = dst->aspectFlags();
            blit.dstSubresource.mipLevel = destinationMip;
            blit.dstSubresource.baseArrayLayer = destinationLayer;
            blit.dstSubresource.layerCount = 1;
            vkCmdBlitImage(_buffer,src->image(),VulkanLib::layout(sourceLayout),dst->image(),VulkanLib::layout(destinationLayout),1,&blit,VulkanLib::filter(filter));
        }

    } // vulkan
} // slag