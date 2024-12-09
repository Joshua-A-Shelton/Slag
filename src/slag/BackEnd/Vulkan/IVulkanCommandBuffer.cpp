#include <iostream>
#include "IVulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"

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
                return GpuQueue::GRAPHICS;
            }
            else if(_family == VulkanLib::card()->computeQueueFamily())
            {
                return GpuQueue::COMPUTE;
            }
            else
            {
                return GpuQueue::TRANSFER;
            }
        }

        VkCommandBuffer IVulkanCommandBuffer::underlyingCommandBuffer()
        {
            return _buffer;
        }

        void IVulkanCommandBuffer::clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "clearColorImage is a graphics queue only operation");
            ImageBarrier barrier{.texture=texture,.oldLayout=currentLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::TRANSFER_WRITE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            auto tex = static_cast<VulkanTexture*>(texture);
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
            barrier.accessAfter = BarrierAccessFlags::COLOR_ATTACHMENT_READ | BarrierAccessFlags::COLOR_ATTACHMENT_WRITE | BarrierAccessFlags::SHADER_READ | BarrierAccessFlags::SHADER_WRITE | BarrierAccessFlags::TRANSFER_READ | BarrierAccessFlags::TRANSFER_WRITE;
            barrier.syncBefore = PipelineStageFlags::TRANSFER;
            barrier.syncAfter = syncAfter;
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        }

        void IVulkanCommandBuffer::updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "updateMipChain is a graphics queue only operation");
            auto tex = static_cast<VulkanTexture*>(texture);
            ImageBarrier barriers[2];
            ;
            barriers[0]={.texture=texture,.baseLayer=0,.layerCount=0,.baseMipLevel=sourceMipLevel,.mipCount=1,.oldLayout=sourceLayout,.newLayout=Texture::TRANSFER_SOURCE,.accessBefore=std::bit_cast<BarrierAccess>(VK_ACCESS_MEMORY_WRITE_BIT),.accessAfter=BarrierAccessFlags::NONE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            barriers[1]={.texture=texture,.baseLayer=0,.layerCount=0,.baseMipLevel=sourceMipLevel+1,.mipCount=0,.oldLayout=destinationLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=std::bit_cast<BarrierAccess>(VK_ACCESS_MEMORY_READ_BIT),.accessAfter=BarrierAccessFlags::NONE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
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
            sourceBarrier.accessAfter = std::bit_cast<BarrierAccess>(VK_ACCESS_MEMORY_READ_BIT);
            sourceBarrier.syncBefore = PipelineStageFlags::TRANSFER;
            sourceBarrier.syncAfter = syncAfter;

            destBarrier.oldLayout = Texture::TRANSFER_DESTINATION;
            destBarrier.newLayout = endingDestinationLayout;
            destBarrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            destBarrier.accessAfter = std::bit_cast<BarrierAccess>(VK_ACCESS_MEMORY_READ_BIT);
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
                auto texture = static_cast<VulkanTexture*>(barrier.texture);
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
                auto buffer = static_cast<VulkanBuffer*>(bufferBarrierDesc.buffer);
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
            VulkanBuffer* src = static_cast<VulkanBuffer*>(source);
            VulkanBuffer* dst = static_cast<VulkanBuffer*>(destination);

            VkBufferCopy copyRegion = {};
            copyRegion.size = length;
            copyRegion.srcOffset = sourceOffset;
            copyRegion.dstOffset = destinationOffset;

            //copy the buffer into the image
            vkCmdCopyBuffer(_buffer, src->underlyingBuffer(), dst->underlyingBuffer(), 1, &copyRegion);
        }

        void IVulkanCommandBuffer::copyImageToBuffer(Texture* texture, Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip, Buffer* buffer,size_t bufferOffset)
        {
            auto tex = static_cast<VulkanTexture*>(texture);
            auto buf = static_cast<VulkanBuffer*>(buffer);
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
            auto image = static_cast<VulkanTexture*>(destination);
            auto buffer = static_cast<VulkanBuffer*>(source);
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
            assert(commandType() == GpuQueue::GRAPHICS && "clearColorImage is a graphics queue only operation");
            auto src = static_cast<VulkanTexture*>(source);
            auto dst = static_cast<VulkanTexture*>(destination);
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

        void IVulkanCommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth)
        {
            VkViewport viewport{};
            viewport.x = x;
            viewport.y = height-y;
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;

            vkCmdSetViewport(_buffer,0,1,&viewport);
        }

        void IVulkanCommandBuffer::setScissors(Rectangle rectangle)
        {
            VkRect2D rect{.offset{rectangle.offset.x,rectangle.offset.y},.extent{rectangle.extent.width,rectangle.extent.height}};
            vkCmdSetScissor(_buffer,0,1,&rect);
        }

        void IVulkanCommandBuffer::beginQuery(QueryPool* queryPool, uint32_t query, bool precise)
        {
            throw std::runtime_error("IVulkanCommandBuffer::beginQuery is not implemented");
        }

        void IVulkanCommandBuffer::beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount, Attachment* depthAttachment,Rectangle bounds)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "beginRendering is a graphics queue only operation");

            std::vector<VkRenderingAttachmentInfo> descriptions(colorAttachmentCount);
            for(auto i=0; i< colorAttachmentCount; i++)
            {
                auto attachment = colorAttachments[i];
                auto colorTexture = static_cast<VulkanTexture*>(attachment.texture);
                descriptions[i]=VkRenderingAttachmentInfo
                {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .imageView = colorTexture->view(),
                    .imageLayout = VulkanLib::layout(attachment.layout),
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VulkanLib::clearValue(attachment.clear)
                };
                if(attachment.clearOnLoad)
                {
                    descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                }
                else
                {
                    descriptions[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }
            }
            VkRenderingAttachmentInfo depth{};
            bool hasStencil = false;
            if(depthAttachment)
            {
                auto depthTex = static_cast<VulkanTexture*>(depthAttachment->texture);
                depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
                depth.imageView = depthTex->view();
                depth.imageLayout = VulkanLib::layout(depthAttachment->layout);
                depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depth.clearValue = VulkanLib::clearValue(depthAttachment->clear);
                if(depthAttachment->clearOnLoad)
                {
                    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                }
                else
                {
                    depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }
                if(Pixels::hasStencilComponent(depthAttachment->texture->format()))
                {
                    hasStencil = true;
                }
            }
            VkRenderingInfoKHR render_info
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .renderArea = {{bounds.offset.x,bounds.offset.y},{bounds.extent.width,bounds.extent.height}},
                .layerCount = 1,
                .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentCount),
                .pColorAttachments = descriptions.data(),
                .pDepthAttachment = depthAttachment == nullptr? nullptr : &depth,
                .pStencilAttachment = hasStencil ? &depth : nullptr
            };
            vkCmdBeginRendering(_buffer,&render_info);
        }

        void IVulkanCommandBuffer::bindGraphicsDescriptorBundle(Shader* shader, uint32_t index, DescriptorBundle& bundle)
        {
            auto s = static_cast<VulkanShader*>(shader);
            auto h = bundle.handle();
            auto handle = std::bit_cast<VkDescriptorSet>(h);
            vkCmdBindDescriptorSets(_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,s->layout(),index,1,&handle,0, nullptr);
        }

        void IVulkanCommandBuffer::bindComputeDescriptorBundle(Shader* shader, uint32_t index, DescriptorBundle& bundle)
        {
            auto s = static_cast<VulkanShader*>(shader);
            auto h = bundle.handle();
            auto handle = std::bit_cast<VkDescriptorSet>(h);
            vkCmdBindDescriptorSets(_buffer,VK_PIPELINE_BIND_POINT_COMPUTE,s->layout(),index,1,&handle,0, nullptr);
        }

        void IVulkanCommandBuffer::bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, size_t offset)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "bindIndexBuffer is a graphics queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);

            vkCmdBindIndexBuffer(_buffer,buf->underlyingBuffer(),offset,VulkanLib::indexType(indexSize));
        }

        void IVulkanCommandBuffer::bindGraphicsShader(Shader* shader)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "bindGraphicsShader is a graphics queue only operation");

            auto pipeLine = static_cast<VulkanShader*>(shader);
            vkCmdBindPipeline(_buffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeLine->pipeline());
        }

        void IVulkanCommandBuffer::bindComputeShader(Shader* shader)
        {
            assert(commandType() != GpuQueue::TRANSFER && "bindComputeShader is a graphics/compute queue only operation");

            auto pipeLine = static_cast<VulkanShader*>(shader);
            vkCmdBindPipeline(_buffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipeLine->pipeline());
        }

        void IVulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, size_t* offsets, size_t bindingCount)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "bindVertexBuffers is a graphics queue only operation");

            std::vector<VkBuffer> nativeBuffers(bindingCount);
            //TODO: I don't like allocating this, possibly change offsets to be uint64_t
            std::vector<VkDeviceSize> nativeOffsets(bindingCount);
            for(size_t i=0; i< nativeBuffers.size(); i++)
            {
                nativeBuffers[i] = static_cast<VulkanBuffer*>(buffers[i])->underlyingBuffer();
                nativeOffsets[i] = offsets[i];
            }
            vkCmdBindVertexBuffers(_buffer,firstBinding,bindingCount,nativeBuffers.data(),nativeOffsets.data());
        }

        void IVulkanCommandBuffer::clearDepthStencilImage(Texture* texture, ClearDepthStencil clear, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore,PipelineStages syncAfter)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "clearDepthStencilImage is a graphics queue only operation");
            ImageBarrier barrier{.texture=texture,.oldLayout=currentLayout,.newLayout=Texture::TRANSFER_DESTINATION,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::TRANSFER_WRITE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::TRANSFER};
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            auto tex = static_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = tex->aspectFlags();
            range.baseArrayLayer = 0;
            range.baseMipLevel = 0;
            range.layerCount = 1;
            range.levelCount = tex->mipLevels();
            vkCmdClearDepthStencilImage(_buffer, tex->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL , reinterpret_cast<const VkClearDepthStencilValue*>(&clear), 1, &range);
            barrier.oldLayout = Texture::TRANSFER_DESTINATION;
            barrier.newLayout = endingLayout;
            barrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
            barrier.accessAfter = BarrierAccessFlags::COLOR_ATTACHMENT_WRITE |  BarrierAccessFlags::SHADER_WRITE | BarrierAccessFlags::TRANSFER_WRITE | BarrierAccessFlags::COLOR_ATTACHMENT_READ |  BarrierAccessFlags::SHADER_READ | BarrierAccessFlags::TRANSFER_READ;
            barrier.syncBefore = PipelineStageFlags::TRANSFER;
            barrier.syncAfter = syncAfter;
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
        }

        void IVulkanCommandBuffer::copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride, QueryPool::ResultFlag flags)
        {
            throw std::runtime_error("IVulkanCommandBuffer::copyQueryPoolResults is not implemented");
        }

        void IVulkanCommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            assert(commandType() != GpuQueue::TRANSFER && "dispatch is a graphics/compute queue only operation");

            vkCmdDispatch(_buffer,groupCountX,groupCountY,groupCountZ);
        }

        void IVulkanCommandBuffer::dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            assert(commandType() != GpuQueue::TRANSFER && "dispatchBase is a graphics/compute queue only operation");

            vkCmdDispatchBase(_buffer,baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
        }

        void IVulkanCommandBuffer::dispatchIndirect(Buffer* buffer, size_t offset)
        {
            assert(commandType() != GpuQueue::TRANSFER && "dispatchIndirect is a graphics/compute queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDispatchIndirect(_buffer,buf->underlyingBuffer(),offset);
        }

        void IVulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "draw is a graphics queue only operation");

            vkCmdDraw(_buffer,vertexCount,instanceCount,firstVertex,firstInstance);
        }

        void IVulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "drawIndexed is a graphics queue only operation");

            vkCmdDrawIndexed(_buffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
        }

        void IVulkanCommandBuffer::drawIndexedIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "drawIndexedIndirect is a graphics queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDrawIndexedIndirect(_buffer,buf->underlyingBuffer(),offset,drawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndexedIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "drawIndexedIndirectCount is a graphics queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);
            auto countBuf = static_cast<VulkanBuffer*>(countBuffer);
            vkCmdDrawIndexedIndirectCount(_buffer,buf->underlyingBuffer(),offset,countBuf->underlyingBuffer(),countBufferOffset,maxDrawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "drawIndirect is a graphics queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDrawIndirect(_buffer,buf->underlyingBuffer(),offset,drawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "drawIndirectCount is a graphics queue only operation");

            auto buf = static_cast<VulkanBuffer*>(buffer);
            auto countBuf = static_cast<VulkanBuffer*>(countBuffer);
            vkCmdDrawIndirectCount(_buffer,buf->underlyingBuffer(),offset,countBuf->underlyingBuffer(),countBufferOffset,maxDrawCount,stride);
        }

        void IVulkanCommandBuffer::endQuery(QueryPool* pool, uint32_t query)
        {
            throw std::runtime_error("IVulkanCommandBuffer::endQuery is not implemented");
        }

        void IVulkanCommandBuffer::endRendering()
        {
            assert(commandType() == GpuQueue::GRAPHICS && "endRendering is a graphics queue only operation");
            vkCmdEndRendering(_buffer);
        }

        void IVulkanCommandBuffer::fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)
        {
            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdFillBuffer(_buffer,buf->underlyingBuffer(),offset,length,data);
        }

        void IVulkanCommandBuffer::resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)
        {
            throw std::runtime_error("IVulkanCommandBuffer::resetQueryPool is not implemented");
        }


        /*void IVulkanCommandBuffer::resolve(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)
        {
            assert(commandType() == GpuQueue::Graphics && "clearColorImage is a graphics queue only operation");
            auto src = static_cast<VulkanTexture*>(source);
            auto dst = static_cast<VulkanTexture*>(destination);
            VkImageResolve blit{};
            blit.extent
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
            vkCmdResolveImage(_buffer,src->image(),VulkanLib::layout(sourceLayout),dst->image(),VulkanLib::layout(destinationLayout),1,&blit);
        }*/

    } // vulkan
} // slag