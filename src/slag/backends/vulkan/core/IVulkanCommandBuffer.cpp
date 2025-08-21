#include "IVulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanShaderPipeline.h"
#include "VulkanTexture.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        GPUQueue::QueueType IVulkanCommandBuffer::commandType()
        {
            return _type;
        }

        void IVulkanCommandBuffer::begin()
        {
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(_commandBuffer,&cmdBeginInfo);
        }

        void IVulkanCommandBuffer::end()
        {
            vkEndCommandBuffer(_commandBuffer);
#ifdef SLAG_DEBUG
            _boundDescriptorPool = nullptr;
            _boundVulkanComputePipelineLayout = nullptr;
            _boundVulkanComputePipelineLayout = nullptr;
            _setViewport = false;
            _setScissor = false;
#endif

        }
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        void IVulkanCommandBuffer::insertBarriers(TextureBarrier* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GlobalBarrier* memoryBarriers, size_t memoryBarrierCount)
        {
            std::vector<VkImageMemoryBarrier2> imageMemoryBarriers(textureBarrierCount,VkImageMemoryBarrier2{});
            for(size_t i=0; i< textureBarrierCount; i++)
            {
                auto& vkbarrier = imageMemoryBarriers[i];
                auto barrier = textureBarriers[i];
                auto texture = static_cast<VulkanTexture*>(barrier.texture);
                vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                vkbarrier.srcAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(barrier.accessBefore);
                vkbarrier.dstAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(barrier.accessAfter);
                vkbarrier.image = texture->vulkanHandle();
                vkbarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.srcStageMask = VulkanBackend::vulkanizedStageMask(barrier.syncBefore);
                vkbarrier.dstStageMask = VulkanBackend::vulkanizedStageMask(barrier.syncAfter);
                vkbarrier.subresourceRange = {.aspectMask = VulkanBackend::vulkanizedAspectFlags(Pixels::aspectFlags(texture->format())), .baseMipLevel =barrier.baseMipLevel, .levelCount = barrier.mipCount == 0 ? texture->mipLevels()-barrier.baseMipLevel : barrier.mipCount, .baseArrayLayer = barrier.baseLayer, .layerCount = barrier.layerCount == 0 ? texture->layers()-barrier.baseLayer : barrier.layerCount};

            }
            std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers(bufferBarrierCount,VkBufferMemoryBarrier2{});
            for(size_t i=0; i< bufferBarrierCount; i++)
            {
                auto& bufferBarrier = bufferMemoryBarriers[i];
                auto bufferBarrierDesc = bufferBarriers[i];
                auto buffer = static_cast<VulkanBuffer*>(bufferBarrierDesc.buffer);
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                bufferBarrier.buffer = buffer->vulkanHandle();
                bufferBarrier.srcAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(bufferBarrierDesc.accessBefore);
                bufferBarrier.dstAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(bufferBarrierDesc.accessAfter);
                bufferBarrier.offset = bufferBarrierDesc.offset;
                bufferBarrier.size = bufferBarrierDesc.size != 0 ? bufferBarrierDesc.size : VK_WHOLE_SIZE;
                bufferBarrier.srcStageMask = VulkanBackend::vulkanizedStageMask(bufferBarrierDesc.syncBefore);
                bufferBarrier.dstStageMask = VulkanBackend::vulkanizedStageMask(bufferBarrierDesc.syncAfter);
            }
            std::vector<VkMemoryBarrier2> memBarriers(memoryBarrierCount,VkMemoryBarrier2{});
            for(size_t i=0; i< memoryBarrierCount; i++)
            {
                auto& memoryBarrier = memBarriers[i];
                auto& memoryBarrierDesc = memoryBarriers[i];
                memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                memoryBarrier.srcAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(memoryBarrierDesc.accessBefore);
                memoryBarrier.dstAccessMask = VulkanBackend::vulkanizedBarrierAccessMask(memoryBarrierDesc.accessAfter);
                memoryBarrier.srcStageMask = VulkanBackend::vulkanizedStageMask(memoryBarrierDesc.syncBefore);
                memoryBarrier.dstStageMask = VulkanBackend::vulkanizedStageMask(memoryBarrierDesc.syncAfter);
            }
            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            //dependencyInfo.dependencyFlags = ;
            dependencyInfo.memoryBarrierCount = memoryBarrierCount;
            dependencyInfo.pMemoryBarriers = memBarriers.data();
            dependencyInfo.bufferMemoryBarrierCount = bufferBarrierCount;
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = textureBarrierCount;
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        void IVulkanCommandBuffer::insertBarrier(const TextureBarrier& barrier)
        {
            insertBarriers(const_cast<TextureBarrier*>(&barrier),1,nullptr,0,nullptr,0);
        }

        void IVulkanCommandBuffer::clearTexture(Texture* texture, ClearColor color)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(!_inRenderPass && "Cleared texture in renderpass (between beginRendering() and endRendering())");
#endif
            SLAG_ASSERT(texture!=nullptr && (bool)(Pixels::aspectFlags(texture->format()) & Pixels::AspectFlags::COLOR) && "Texture must be a color texture");
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseArrayLayer = 0;
            range.layerCount = vulkanTexture->layers();
            range.baseMipLevel = 0;
            range.levelCount = vulkanTexture->mipLevels();
            vkCmdClearColorImage(_commandBuffer,vulkanTexture->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,reinterpret_cast<const VkClearColorValue*>(&color),1,&range);
        }

        void IVulkanCommandBuffer::clearTexture(Texture* texture, ClearDepthStencilValue depthStencil)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(!_inRenderPass && "Cleared texture in renderpass (between beginRendering() and endRendering())");
#endif
            SLAG_ASSERT(texture!=nullptr && (bool)(Pixels::aspectFlags(texture->format()) & Pixels::AspectFlags::DEPTH) && "Texture must be a depth texture");
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VulkanBackend::vulkanizedAspectFlags(Pixels::aspectFlags(texture->format()));
            range.baseArrayLayer = 0;
            range.layerCount = vulkanTexture->layers();
            range.baseMipLevel = 0;
            range.levelCount = vulkanTexture->mipLevels();
            vkCmdClearDepthStencilImage(_commandBuffer,vulkanTexture->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,reinterpret_cast<const VkClearDepthStencilValue*>(&depthStencil),1,&range);
        }

        void IVulkanCommandBuffer::updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip, uint32_t destinationMip)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(!_inRenderPass && "Updated mip in renderpass (between beginRendering() and endRendering()");
#endif

            SLAG_ASSERT(texture!=nullptr && "texture cannot be null");
            SLAG_ASSERT(texture->mipLevels()>destinationMip && "Destination mip more than texture has");
            SLAG_ASSERT(sourceMip<destinationMip && "Source mip is greater than destination mip");
            SLAG_ASSERT(texture->layers()>layer && "Layer is greater than texture has");
            blit(texture,layer,sourceMip,Rectangle{.offset = {0,0},.extent = {texture->width(sourceMip),texture->height(sourceMip)}},texture,layer,destinationMip,Rectangle{.offset = {0,0},.extent = {texture->width(destinationMip),texture->height(destinationMip)}});
        }

        void IVulkanCommandBuffer::copyTextureToBuffer(Texture* source, TextureToBufferCopyData* copyData, uint32_t subresourceCount, Buffer* destination)
        {
            SLAG_ASSERT(source!=nullptr && "texture cannot be null");
            SLAG_ASSERT(destination!=nullptr && "buffer cannot be null");
            SLAG_ASSERT(copyData!=nullptr && "copyData cannot be null");
            SLAG_ASSERT(subresourceCount>0 && "subresource count cannot be 0");

            auto vulkanTexture = static_cast<VulkanTexture*>(source);
            auto vulkanBuffer = static_cast<VulkanBuffer*>(destination);

            std::vector<VkBufferImageCopy> regions(subresourceCount);
            auto aspectMask = VulkanBackend::vulkanizedAspectFlags(Pixels::aspectFlags(source->format()));
            for (uint32_t i = 0; i < subresourceCount; ++i)
            {
                auto& region = regions[i];
                auto& subResource = copyData[i];
                SLAG_ASSERT(subResource.bufferOffset % Pixels::size(source->format())==0 && "Offset into buffer must be multiple of pixel size");
                region.bufferOffset = subResource.bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = aspectMask;
                region.imageSubresource.mipLevel = subResource.subresource.mipLevel;
                region.imageSubresource.baseArrayLayer = subResource.subresource.baseArrayLayer;
                region.imageSubresource.layerCount = subResource.subresource.layerCount;
                region.imageOffset = {0,0,0};
                region.imageExtent = {source->width(subResource.subresource.mipLevel),source->height(subResource.subresource.mipLevel),1};
            }

            vkCmdCopyImageToBuffer(_commandBuffer,vulkanTexture->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,vulkanBuffer->vulkanHandle(),subresourceCount,regions.data());
        }

        void IVulkanCommandBuffer::copyBufferToTexture(Buffer* source, uint64_t offset, Texture* destination,TextureSubresource subresource)
        {
            SLAG_ASSERT(source!=nullptr && "buffer cannot be null");
            SLAG_ASSERT(destination!=nullptr && "texture cannot be null");


            auto image = static_cast<VulkanTexture*>(destination);
            auto buffer = static_cast<VulkanBuffer*>(source);

            VkBufferImageCopy copy{};
            copy.imageExtent = {.width=destination->width(subresource.mipLevel),.height=destination->height(subresource.mipLevel),.depth=1};
            copy.bufferOffset = offset;

            copy.imageSubresource =
            {
                .aspectMask = VulkanBackend::vulkanizedAspectFlags(subresource.aspectFlags),
                .mipLevel = subresource.mipLevel,
                .baseArrayLayer = subresource.baseArrayLayer,
                .layerCount = subresource.layerCount
            };

            vkCmdCopyBufferToImage(_commandBuffer,buffer->vulkanHandle(),image->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,1,&copy);
        }

        void IVulkanCommandBuffer::blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea)
        {
            SLAG_ASSERT(source!=nullptr && "source cannot be null");
            blit(source,sourceLayer,sourceMip,sourceArea,destination,destinationLayer,destinationMip,destinationArea,Pixels::aspectFlags(source->format()));
        }

        void IVulkanCommandBuffer::blit(Texture* source, uint32_t sourceLayer,uint32_t sourceMip, Rectangle sourceArea, Texture* destination, uint32_t destinationLayer,uint32_t destinationMip, Rectangle destinationArea, Pixels::AspectFlags aspects)
        {
            SLAG_ASSERT(source!=nullptr && "source cannot be null");
            SLAG_ASSERT(destination!=nullptr && "destination cannot be null");
            SLAG_ASSERT(sourceLayer < source->layers() && "source layer exceeds source layer count");
            SLAG_ASSERT(sourceMip < source->mipLevels() && "source mip exceeds source mip count");
            SLAG_ASSERT((bool)(Pixels::aspectFlags(source->format()) & aspects) && "source must have requested aspects to blit");
            SLAG_ASSERT((bool)(Pixels::aspectFlags(destination->format()) & aspects) && "destination must have requested aspects to blit");

            auto src = static_cast<VulkanTexture*>(source);
            auto dst = static_cast<VulkanTexture*>(destination);

            auto aspectFlags = VulkanBackend::vulkanizedAspectFlags(aspects);

            VkImageBlit blit{};
            blit.srcOffsets[0] = {sourceArea.offset.x,sourceArea.offset.y,0};
            blit.srcOffsets[1] = {static_cast<int32_t>(sourceArea.extent.width + sourceArea.offset.x),static_cast<int32_t>(sourceArea.extent.height + sourceArea.offset.y),1};
            blit.srcSubresource.aspectMask = aspectFlags;
            blit.srcSubresource.mipLevel = sourceMip;
            blit.srcSubresource.baseArrayLayer = sourceLayer;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {destinationArea.offset.x,destinationArea.offset.y,0};
            blit.dstOffsets[1] = {static_cast<int32_t>(destinationArea.extent.width + destinationArea.offset.x),static_cast<int32_t>(destinationArea.extent.height + destinationArea.offset.y),1};
            blit.dstSubresource.aspectMask = aspectFlags;
            blit.dstSubresource.mipLevel = destinationMip;
            blit.dstSubresource.baseArrayLayer = destinationLayer;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(_commandBuffer,src->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,dst->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,1,&blit,VK_FILTER_NEAREST);
        }

        void IVulkanCommandBuffer::resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Offset destinationOffset, Extent resolveExtent)
        {
            SLAG_ASSERT(destination!=nullptr && "destination cannot be null");
            SLAG_ASSERT(sourceLayer < source->layers() && "source layer exceeds source layer count");
            SLAG_ASSERT(sourceMip < source->mipLevels() && "source mip exceeds source mip count");
            SLAG_ASSERT((bool)(Pixels::aspectFlags(source->format()) & Pixels::AspectFlags::COLOR) && "source must be color image to resolve");
            SLAG_ASSERT((bool)(Pixels::aspectFlags(destination->format()) & Pixels::AspectFlags::COLOR) && "destination must be color image to resolve");

            auto src = static_cast<VulkanTexture*>(source);
            auto dst = static_cast<VulkanTexture*>(destination);

            VkImageResolve resolve{};
            resolve.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            resolve.srcSubresource.mipLevel = sourceMip;
            resolve.srcSubresource.baseArrayLayer = sourceLayer;
            resolve.srcSubresource.layerCount = 1;
            resolve.srcOffset = {sourceOffset.x,sourceOffset.y,0};
            resolve.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            resolve.dstSubresource.mipLevel = destinationMip;
            resolve.dstSubresource.baseArrayLayer = destinationLayer;
            resolve.dstSubresource.layerCount = 1;
            resolve.dstOffset = {destinationOffset.x,destinationOffset.y,0};
            resolve.extent = {resolveExtent.width,resolveExtent.height,1};
            vkCmdResolveImage(_commandBuffer,src->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,dst->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,1,&resolve);
        }

        void IVulkanCommandBuffer::beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment, Rectangle bounds)
        {
            SLAG_ASSERT(colorAttachments != nullptr && "colorAttachment cannot be null");
            SLAG_ASSERT(colorAttachmentCount > 0 && "color attachment count must be positive");
#ifdef SLAG_DEBUG
            SLAG_ASSERT(!_inRenderPass && "Cannot start renderpass while inside another renderpass, call to endRendering must be made first");
            _inRenderPass = true;
#endif

            std::vector<VkRenderingAttachmentInfo> descriptions(colorAttachmentCount);
            for(auto i=0; i< colorAttachmentCount; i++)
            {

                auto attachment = colorAttachments[i];
                auto colorTexture = static_cast<VulkanTexture*>(attachment.texture);
                SLAG_ASSERT(colorTexture != nullptr && "color texture cannot be null");
                SLAG_ASSERT((bool)(colorTexture->usageFlags() & Texture::UsageFlags::RENDER_TARGET_ATTACHMENT) && "Color attachment without Texture::UsageFlags::RENDER_TARGET_ATTACHMENT provided");
                descriptions[i]=VkRenderingAttachmentInfo
                {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .imageView = colorTexture->vulkanViewHandle(),
                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = std::bit_cast<VkClearValue>(attachment.clearValue)
                };
                if(attachment.autoClear)
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
                depth.imageView = depthTex->vulkanViewHandle();
                depth.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depth.clearValue = std::bit_cast<VkClearValue>(depthAttachment->clearValue);
                if(depthAttachment->autoClear)
                {
                    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                }
                else
                {
                    depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                }
                if(static_cast<bool>(Pixels::aspectFlags(depthAttachment->texture->format()) & Pixels::AspectFlags::STENCIL))
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
            vkCmdBeginRendering(_commandBuffer,&render_info);
        }

#else
#endif

        void IVulkanCommandBuffer::insertBarrier(const BufferBarrier& barrier)
        {
            insertBarriers(nullptr,0,const_cast<BufferBarrier*>(&barrier),1,nullptr,0);
        }

        void IVulkanCommandBuffer::insertBarrier(const GlobalBarrier& barrier)
        {
            insertBarriers(nullptr,0,nullptr,0,const_cast<GlobalBarrier*>(&barrier),1);
        }

        void IVulkanCommandBuffer::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length,Buffer* destination, uint64_t destinationOffset)
        {
            SLAG_ASSERT(source != nullptr && "Source buffer cannot be null");
            SLAG_ASSERT(destination != nullptr && "Destination buffer cannot be null");
            SLAG_ASSERT(sourceOffset+length <= source->size() && "Copy would exceed length of source buffer");
            SLAG_ASSERT(destinationOffset+length <= destination->size() && "Copy would exceed destination buffer");

            VulkanBuffer* src = static_cast<VulkanBuffer*>(source);
            VulkanBuffer* dst = static_cast<VulkanBuffer*>(destination);

            VkBufferCopy copyRegion = {};
            copyRegion.size = length;
            copyRegion.srcOffset = sourceOffset;
            copyRegion.dstOffset = destinationOffset;

            //copy the buffer into the image
            vkCmdCopyBuffer(_commandBuffer, src->vulkanHandle(), dst->vulkanHandle(), 1, &copyRegion);
        }

        void IVulkanCommandBuffer::fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)
        {
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT(offset + length <= buffer->size() && "Fill would exceed length of buffer");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdFillBuffer(_commandBuffer,buf->vulkanHandle(),offset,length,data);
        }

        void IVulkanCommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)
        {
            VkViewport viewport{};
            viewport.x = x;
            viewport.y = height-y;
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;

            vkCmdSetViewport(_commandBuffer,0,1,&viewport);
            _setViewport = true;
        }

        void IVulkanCommandBuffer::setScissors(Rectangle rectangle)
        {
            VkRect2D rect{.offset{rectangle.offset.x,rectangle.offset.y},.extent{rectangle.extent.width,rectangle.extent.height}};
            vkCmdSetScissor(_commandBuffer,0,1,&rect);
            _setScissor = true;
        }

        void IVulkanCommandBuffer::setBlendConstants(float r, float g, float b, float a)
        {
            float floats[4];
            floats[0]=r;
            floats[1]=g;
            floats[2]=b;
            floats[3]=a;
            vkCmdSetBlendConstants(_commandBuffer,floats);
        }

        void IVulkanCommandBuffer::setStencilReference(uint32_t reference)
        {
            vkCmdSetStencilReference(_commandBuffer,VK_STENCIL_FACE_FRONT_AND_BACK,reference);
        }

        void IVulkanCommandBuffer::endRendering()
        {
            vkCmdEndRendering(_commandBuffer);
#if SLAG_DEBUG
            _inRenderPass = false;
#endif
        }

        void IVulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif

            vkCmdDraw(_commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
        }

        void IVulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            vkCmdDrawIndexed(_commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
        }

        void IVulkanCommandBuffer::drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount,uint32_t stride)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT(offset + drawCount*stride <= buffer->size() && "drawIndexedIndirect will exceed length of buffer");
            SLAG_ASSERT((bool)(buffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "buffer must have been created with usage flag INDIRECT_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDrawIndexedIndirect(_commandBuffer,buf->vulkanHandle(),offset,drawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT(countBuffer != nullptr && "CountBuffer cannot be null");
            SLAG_ASSERT(offset + maxDrawCount*stride <= buffer->size() && "drawIndexedIndirect can exceed length of buffer");
            SLAG_ASSERT((bool)(buffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "buffer must have been created with usage flag INDIRECT_BUFFER");
            SLAG_ASSERT((bool)(countBuffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "countBuffer must have been created with usage flag INDIRECT_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            auto countBuf = static_cast<VulkanBuffer*>(countBuffer);
            vkCmdDrawIndexedIndirectCount(_commandBuffer,buf->vulkanHandle(),offset,countBuf->vulkanHandle(),countBufferOffset,maxDrawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT(offset + drawCount*stride <= buffer->size() && "drawIndirect will exceed length of buffer");
            SLAG_ASSERT((bool)(buffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "buffer must have been created with usage flag INDIRECT_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDrawIndirect(_commandBuffer,buf->vulkanHandle(),offset,drawCount,stride);
        }

        void IVulkanCommandBuffer::drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT(countBuffer != nullptr && "CountBuffer cannot be null");
            SLAG_ASSERT(offset + maxDrawCount*stride <= buffer->size() && "drawIndexedIndirect can exceed length of buffer");
            SLAG_ASSERT((bool)(buffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "buffer must have been created with usage flag INDIRECT_BUFFER");
            SLAG_ASSERT((bool)(countBuffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "countBuffer must have been created with usage flag INDIRECT_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            auto countBuf = static_cast<VulkanBuffer*>(countBuffer);
            vkCmdDrawIndirectCount(_commandBuffer,buf->vulkanHandle(),offset,countBuf->vulkanHandle(),countBufferOffset,maxDrawCount,stride);
        }

        void IVulkanCommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            vkCmdDispatch(_commandBuffer,groupCountX,groupCountY,groupCountZ);
        }

        void IVulkanCommandBuffer::dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            vkCmdDispatchBase(_commandBuffer,baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
        }

        void IVulkanCommandBuffer::dispatchIndirect(Buffer* buffer, size_t offset)
        {
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT((bool)(buffer->usage() & Buffer::UsageFlags::INDIRECT_BUFFER) && "buffer must have been created with usage flag INDIRECT_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);
            vkCmdDispatchIndirect(_commandBuffer,buf->vulkanHandle(),offset);
        }

        void IVulkanCommandBuffer::bindGraphicsShaderPipeline(ShaderPipeline* pipeline)
        {
            auto pipeLine = static_cast<VulkanShaderPipeline*>(pipeline);
            _boundVulkanGraphicsShaderPipelineLayout = pipeLine->vulkanLayout();
            vkCmdBindPipeline(_commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeLine->vulkanHandle());
        }

        void IVulkanCommandBuffer::bindComputeShaderPipeline(ShaderPipeline* pipeline)
        {
            auto pipeLine = static_cast<VulkanShaderPipeline*>(pipeline);
            _boundVulkanComputePipelineLayout = pipeLine->vulkanLayout();
            vkCmdBindPipeline(_commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,pipeLine->vulkanHandle());
        }

        void IVulkanCommandBuffer::bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle)
        {
#ifdef SLAG_DEBUG
            SLAG_ASSERT(bundle.cpuHandle() == _boundDescriptorPool && "Cannot bind descriptor from descriptor pool which is not currently bound");
#endif

            SLAG_ASSERT(_boundVulkanGraphicsShaderPipelineLayout != nullptr && "No graphics shader is bound, unable to bind descriptor bundle");
            auto h = bundle.gpuHandle();
            auto handle = std::bit_cast<VkDescriptorSet>(h);
            vkCmdBindDescriptorSets(_commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, _boundVulkanGraphicsShaderPipelineLayout,index,1,&handle,0, nullptr);
        }

        void IVulkanCommandBuffer::bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle)
        {
#ifdef SLAG_DEBUG
            SLAG_ASSERT(bundle.cpuHandle() == _boundDescriptorPool && "Cannot bind descriptor from descriptor pool which is not currently bound");
#endif
            SLAG_ASSERT(_boundVulkanComputePipelineLayout != nullptr && "No graphics shader is bound, unable to bind descriptor bundle");
            auto h = bundle.gpuHandle();
            auto handle = std::bit_cast<VkDescriptorSet>(h);
            vkCmdBindDescriptorSets(_commandBuffer,VK_PIPELINE_BIND_POINT_COMPUTE,_boundVulkanComputePipelineLayout,index,1,&handle,0, nullptr);

        }

        void IVulkanCommandBuffer::bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset)
        {
            SLAG_ASSERT(buffer != nullptr && "Buffer cannot be null");
            SLAG_ASSERT((bool)(buffer->usage()&Buffer::UsageFlags::INDEX_BUFFER) && "buffer must have been created with usage flag INDEX_BUFFER");
            auto buf = static_cast<VulkanBuffer*>(buffer);

            vkCmdBindIndexBuffer(_commandBuffer,buf->vulkanHandle(),offset,VulkanBackend::vulkanizedIndexType(indexSize));
        }

        void IVulkanCommandBuffer::bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers,uint64_t* bufferOffsets,uint64_t* strides, size_t bufferCount)
        {
            SLAG_ASSERT(buffers != nullptr && "Buffers cannot be null");
            SLAG_ASSERT(bufferCount > 0 && "Buffer Count must be greater than 0");
            std::vector<VkBuffer> nativeBuffers(bufferCount);
            for(size_t i=0; i< nativeBuffers.size(); i++)
            {
                nativeBuffers[i] = static_cast<VulkanBuffer*>(buffers[i])->vulkanHandle();
            }
            vkCmdBindVertexBuffers2(_commandBuffer,firstBindingIndex,bufferCount,nativeBuffers.data(),bufferOffsets,nullptr,strides);
        }

        void IVulkanCommandBuffer::transitionToLayout(Texture* texture, VkImageLayout oldLayout,VkImageLayout newLayout, VkAccessFlags2 accessBefore, VkAccessFlags2 accessAfter, VkPipelineStageFlags2 syncBefore, VkPipelineStageFlags2 syncAfter) const
        {
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkImageMemoryBarrier2 barrier{};

            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            barrier.pNext = nullptr,
            barrier.srcStageMask = syncBefore,
            barrier.srcAccessMask = accessBefore,
            barrier.dstStageMask = syncAfter,
            barrier.dstAccessMask = accessAfter,
            barrier.oldLayout = oldLayout,
            barrier.newLayout = newLayout,
            barrier.image = vulkanTexture->vulkanHandle(),
            barrier.subresourceRange = VkImageSubresourceRange
            {
                .aspectMask = VulkanBackend::vulkanizedAspectFlags(Pixels::aspectFlags(texture->format())),
                .baseMipLevel = 0,
                .levelCount = texture->mipLevels(),
                .baseArrayLayer = 0,
                .layerCount = texture->layers(),
            };


            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            //dependencyInfo.dependencyFlags = ;
            dependencyInfo.memoryBarrierCount = 0;
            dependencyInfo.pMemoryBarriers = nullptr;
            dependencyInfo.bufferMemoryBarrierCount = 0;
            dependencyInfo.pBufferMemoryBarriers = nullptr;
            dependencyInfo.imageMemoryBarrierCount = 1;
            dependencyInfo.pImageMemoryBarriers = &barrier;
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        VkCommandBuffer IVulkanCommandBuffer::vulkanCommandBufferHandle()
        {
            return _commandBuffer;
        }

        VkCommandPool IVulkanCommandBuffer::vulkanCommandPoolHandle()
        {
            return _pool;
        }
    } // vulkan
} // slag
