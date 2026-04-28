#include "IVulkanCommandBuffer.h"

#include <bit>

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorHeap.h"
#include "VulkanGraphicsCard.h"
#include "VulkanShaderPipeline.h"
#include "VulkanTexture.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;

        GraphicsCard* IVulkanCommandBuffer::graphicsCard()
        {
            return _graphicsCard;
        }

        QueueType IVulkanCommandBuffer::type() const
        {
            return _type;
        }

        void IVulkanCommandBuffer::begin()
        {
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            //TODO: I need to have this set to match DX12 functionality
            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(_commandBuffer,&cmdBeginInfo);
        }

        void IVulkanCommandBuffer::end()
        {
            vkEndCommandBuffer(_commandBuffer);
#ifdef SLAG_DEBUG
            _setViewport = false;
            _setScissor = false;
#endif
        }

        void IVulkanCommandBuffer::insertBarriers(GlobalBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");
            std::vector<VkMemoryBarrier2> memBarriers(barrierCount,VkMemoryBarrier2{});
            for(size_t i=0; i< barrierCount; i++)
            {
                auto& memoryBarrier = memBarriers[i];
                auto& memoryBarrierDesc = barriers[i];
                memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                memoryBarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(memoryBarrierDesc.flush);
                memoryBarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(memoryBarrierDesc.invalidate);
                memoryBarrier.srcStageMask = VulkanBackend::nativePipelineStages(memoryBarrierDesc.syncBefore);
                memoryBarrier.dstStageMask = VulkanBackend::nativePipelineStages(memoryBarrierDesc.syncAfter);
            }
            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.memoryBarrierCount = barrierCount;
            dependencyInfo.pMemoryBarriers = memBarriers.data();
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        void IVulkanCommandBuffer::insertBarriers(BufferBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");
            std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers(barrierCount,VkBufferMemoryBarrier2{});
            for(size_t i=0; i< barrierCount; i++)
            {
                auto& bufferBarrier = bufferMemoryBarriers[i];
                auto bufferBarrierDesc = barriers[i];
                auto buffer = static_cast<VulkanBuffer*>(bufferBarrierDesc.buffer);
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                bufferBarrier.buffer = buffer->vulkanHandle();
                bufferBarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(bufferBarrierDesc.flush);
                bufferBarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(bufferBarrierDesc.invalidate);
                bufferBarrier.offset = bufferBarrierDesc.offset;
                bufferBarrier.size = bufferBarrierDesc.length != 0 ? bufferBarrierDesc.length : VK_WHOLE_SIZE;
                bufferBarrier.srcStageMask = VulkanBackend::nativePipelineStages(bufferBarrierDesc.syncBefore);
                bufferBarrier.dstStageMask = VulkanBackend::nativePipelineStages(bufferBarrierDesc.syncAfter);
            }

            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.bufferMemoryBarrierCount = barrierCount;
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        void IVulkanCommandBuffer::insertBarriers(TextureBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");
            std::vector<VkImageMemoryBarrier2> imageMemoryBarriers(barrierCount,VkImageMemoryBarrier2{});
            for(size_t i=0; i< barrierCount; i++)
            {
                auto& vkbarrier = imageMemoryBarriers[i];
                auto barrier = barriers[i];
                auto texture = static_cast<VulkanTexture*>(barrier.texture);
                vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                vkbarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(barrier.flush);
                vkbarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(barrier.invalidate);
                vkbarrier.image = texture->vulkanHandle();
                vkbarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.srcStageMask = VulkanBackend::nativePipelineStages(barrier.syncBefore);
                vkbarrier.dstStageMask = VulkanBackend::nativePipelineStages(barrier.syncAfter);
                vkbarrier.subresourceRange = {.aspectMask = VulkanBackend::nativeTextureAspect(Pixel::aspectFlags(texture->format())), .baseMipLevel =barrier.baseMipLevel, .levelCount = barrier.mipCount == 0 ? texture->mipLevels()-barrier.baseMipLevel : barrier.mipCount, .baseArrayLayer = barrier.baseLayer, .layerCount = barrier.layerCount == 0 ? texture->layers()-barrier.baseLayer : barrier.layerCount};

            }

            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.imageMemoryBarrierCount = barrierCount;
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        void IVulkanCommandBuffer::insertBarriers(
            GlobalBarrier* globalBarriers,
            uint32_t globalBarrierCount,
            BufferBarrier* bufferBarriers,
            uint32_t bufferBarrierCount,
            TextureBarrier* textureBarriers,
            uint32_t textureBarrierCount)
        {
            std::vector<VkImageMemoryBarrier2> imageMemoryBarriers(textureBarrierCount,VkImageMemoryBarrier2{});
            for(size_t i=0; i< textureBarrierCount; i++)
            {
                auto& vkbarrier = imageMemoryBarriers[i];
                auto barrier = textureBarriers[i];
                auto texture = static_cast<VulkanTexture*>(barrier.texture);
                vkbarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
                vkbarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(barrier.flush);
                vkbarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(barrier.invalidate);
                vkbarrier.image = texture->vulkanHandle();
                vkbarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                vkbarrier.srcStageMask = VulkanBackend::nativePipelineStages(barrier.syncBefore);
                vkbarrier.dstStageMask = VulkanBackend::nativePipelineStages(barrier.syncAfter);
                vkbarrier.subresourceRange = {.aspectMask = VulkanBackend::nativeTextureAspect(Pixel::aspectFlags(texture->format())), .baseMipLevel =barrier.baseMipLevel, .levelCount = barrier.mipCount == 0 ? texture->mipLevels()-barrier.baseMipLevel : barrier.mipCount, .baseArrayLayer = barrier.baseLayer, .layerCount = barrier.layerCount == 0 ? texture->layers()-barrier.baseLayer : barrier.layerCount};

            }
            std::vector<VkBufferMemoryBarrier2> bufferMemoryBarriers(bufferBarrierCount,VkBufferMemoryBarrier2{});
            for(size_t i=0; i< bufferBarrierCount; i++)
            {
                auto& bufferBarrier = bufferMemoryBarriers[i];
                auto bufferBarrierDesc = bufferBarriers[i];
                auto buffer = static_cast<VulkanBuffer*>(bufferBarrierDesc.buffer);
                bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
                bufferBarrier.buffer = buffer->vulkanHandle();
                bufferBarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(bufferBarrierDesc.flush);
                bufferBarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(bufferBarrierDesc.invalidate);
                bufferBarrier.offset = bufferBarrierDesc.offset;
                bufferBarrier.size = bufferBarrierDesc.length != 0 ? bufferBarrierDesc.length : VK_WHOLE_SIZE;
                bufferBarrier.srcStageMask = VulkanBackend::nativePipelineStages(bufferBarrierDesc.syncBefore);
                bufferBarrier.dstStageMask = VulkanBackend::nativePipelineStages(bufferBarrierDesc.syncAfter);
            }
            std::vector<VkMemoryBarrier2> memBarriers(globalBarrierCount,VkMemoryBarrier2{});
            for(size_t i=0; i< globalBarrierCount; i++)
            {
                auto& memoryBarrier = memBarriers[i];
                auto& memoryBarrierDesc = globalBarriers[i];
                memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
                memoryBarrier.srcAccessMask = VulkanBackend::nativeMemoryCaches(memoryBarrierDesc.flush);
                memoryBarrier.dstAccessMask = VulkanBackend::nativeMemoryCaches(memoryBarrierDesc.invalidate);
                memoryBarrier.srcStageMask = VulkanBackend::nativePipelineStages(memoryBarrierDesc.syncBefore);
                memoryBarrier.dstStageMask = VulkanBackend::nativePipelineStages(memoryBarrierDesc.syncAfter);
            }
            VkDependencyInfo dependencyInfo{};
            dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dependencyInfo.memoryBarrierCount = globalBarrierCount;
            dependencyInfo.pMemoryBarriers = memBarriers.data();
            dependencyInfo.bufferMemoryBarrierCount = bufferBarrierCount;
            dependencyInfo.pBufferMemoryBarriers = bufferMemoryBarriers.data();
            dependencyInfo.imageMemoryBarrierCount = textureBarrierCount;
            dependencyInfo.pImageMemoryBarriers = imageMemoryBarriers.data();
            vkCmdPipelineBarrier2(_commandBuffer,&dependencyInfo);
        }

        void IVulkanCommandBuffer::bindDescriptorHeaps(DescriptorHeap* resourceHeap, DescriptorHeap* samplerHeap)
        {
            if (resourceHeap)
            {
                auto rHeap = static_cast<VulkanDescriptorHeap*>(resourceHeap);
                SLAG_ASSERT(rHeap->type() == DescriptorHeapType::RESOURCE);
                VkBindHeapInfoEXT bindHeapInfo
                {
                    .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
                    .pNext = nullptr,
                    .heapRange = {.address = rHeap->deviceAddress(), .size = rHeap->size() + rHeap->reserved()},
                    .reservedRangeOffset = rHeap->size(),
                    .reservedRangeSize = rHeap->reserved()
                };
                _graphicsCard->vkCmdBindResourceHeap(_commandBuffer,&bindHeapInfo);
            }
            if (samplerHeap)
            {
                auto sHeap = static_cast<VulkanDescriptorHeap*>(samplerHeap);
                SLAG_ASSERT(sHeap->type() == DescriptorHeapType::SAMPLER);
                VkBindHeapInfoEXT bindHeapInfo
               {
                   .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
                   .pNext = nullptr,
                   .heapRange = {.address = sHeap->deviceAddress(), .size = sHeap->size() + sHeap->reserved()},
                   .reservedRangeOffset = sHeap->size(),
                   .reservedRangeSize = sHeap->reserved()
               };
                _graphicsCard->vkCmdBindSamplerHeap(_commandBuffer,&bindHeapInfo);
            }
        }

        void IVulkanCommandBuffer::setInputBindingTable(uint32_t byteOffset, uint32_t heapOffset)
        {
            VkPushDataInfoEXT pushDataInfo
            {
                .sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
                .pNext = nullptr,
                .offset = byteOffset,
                .data = {.address = &heapOffset,.size = sizeof(heapOffset)}
            };
            _graphicsCard->vkCmdPushData(_commandBuffer,&pushDataInfo);
        }


        void IVulkanCommandBuffer::copyBufferToBuffer(
            Buffer* source,
            uint64_t sourceOffset,
            Buffer* destination,
            uint64_t destinationOffset,
            uint64_t length)
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

            vkCmdCopyBuffer(_commandBuffer, src->vulkanHandle(), dst->vulkanHandle(), 1, &copyRegion);
        }

        void IVulkanCommandBuffer::copyTextureToBuffer(
            Texture* source,
            Buffer* destination,
            TextureBufferMapping* copyData,
            uint32_t mappingCount)
        {
            SLAG_ASSERT(source!=nullptr && "source cannot be null");
            SLAG_ASSERT(destination!=nullptr && "destination cannot be null");
            SLAG_ASSERT(copyData!=nullptr && "copyData cannot be null");
            SLAG_ASSERT(mappingCount>0 && "mappingCount count cannot be 0");

            auto vulkanTexture = static_cast<VulkanTexture*>(source);
            auto vulkanBuffer = static_cast<VulkanBuffer*>(destination);

            std::vector<VkBufferImageCopy> regions(mappingCount);
            for (uint32_t i = 0; i < mappingCount; ++i)
            {
                auto& region = regions[i];
                auto& subResource = copyData[i];
                auto aspectMask = VulkanBackend::nativeTextureAspect(static_cast<PixelAspectFlags>(subResource.subresource.aspect));
                region.bufferOffset = subResource.bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = aspectMask;
                region.imageSubresource.mipLevel = subResource.subresource.mipLevel;
                region.imageSubresource.baseArrayLayer = subResource.subresource.baseArrayLayer;
                region.imageSubresource.layerCount = subResource.subresource.layerCount;
                region.imageOffset = {.x = subResource.offset.x,.y = subResource.offset.y,.z = subResource.offset.z};
                region.imageExtent = {.width = subResource.extent.width,.height = subResource.extent.height,.depth = subResource.extent.depth};
            }

            vkCmdCopyImageToBuffer(_commandBuffer,vulkanTexture->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,vulkanBuffer->vulkanHandle(),mappingCount,regions.data());
        }

        void IVulkanCommandBuffer::copyBufferToTexture(
            Buffer* source,
            Texture* destination,
            TextureBufferMapping* copyData,
            uint32_t mappingCount)
        {
            SLAG_ASSERT(source!=nullptr && "buffer cannot be null");
            SLAG_ASSERT(destination!=nullptr && "texture cannot be null");
            SLAG_ASSERT(copyData!=nullptr && "copyData cannot be null");
            SLAG_ASSERT(mappingCount>0 && "mappingCount count cannot be 0");


            auto image = static_cast<VulkanTexture*>(destination);
            auto buffer = static_cast<VulkanBuffer*>(source);

            std::vector<VkBufferImageCopy> regions(mappingCount);

            for (uint32_t i = 0; i < mappingCount; ++i)
            {
                auto& region = regions[i];
                auto& subResource = copyData[i];
                auto aspectMask = VulkanBackend::nativeTextureAspect(static_cast<PixelAspectFlags>(subResource.subresource.aspect));
                region.bufferOffset = subResource.bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = aspectMask;
                region.imageSubresource.mipLevel = subResource.subresource.mipLevel;
                region.imageSubresource.baseArrayLayer = subResource.subresource.baseArrayLayer;
                region.imageSubresource.layerCount = subResource.subresource.layerCount;
                region.imageOffset = {.x = subResource.offset.x,.y = subResource.offset.y,.z = subResource.offset.z};
                region.imageExtent = {.width = subResource.extent.width,.height = subResource.extent.height,.depth = subResource.extent.depth};
            }

            vkCmdCopyBufferToImage(_commandBuffer,buffer->vulkanHandle(),image->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,mappingCount,regions.data());
        }

        void IVulkanCommandBuffer::bindGraphicsPipeline(ShaderPipeline* pipeline)
        {
            auto vulkanPipeline = static_cast<VulkanShaderPipeline*>(pipeline);
            vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->vulkanHandle());
        }

        void IVulkanCommandBuffer::beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,
            Attachment* depthAttachment, const Rectangle& bounds)
        {
            std::vector<VkRenderingAttachmentInfo> descriptions(colorAttachmentCount);
            for(auto i=0; i< colorAttachmentCount; i++)
            {

                auto attachment = colorAttachments[i];
                auto colorTexture = static_cast<VulkanTexture*>(attachment.texture);
                SLAG_ASSERT(colorTexture != nullptr && "color texture cannot be null");
                SLAG_ASSERT((bool)(colorTexture->usage() & TextureUsageFlags::COLOR_TARGET) && "Color attachment without Texture::UsageFlags::RENDER_TARGET_ATTACHMENT provided");
                descriptions[i]=VkRenderingAttachmentInfo
                {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .imageView = colorTexture->vulkanView(),
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
                depth.imageView = depthTex->vulkanView();
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
                if(static_cast<bool>(Pixel::aspectFlags(depthAttachment->texture->format()) & PixelAspectFlags::STENCIL_FLAG))
                {
                    hasStencil = true;
                }
            }
            VkRenderingInfoKHR render_info
            {
                .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .renderArea = {{bounds.offset.x,bounds.offset.y},{bounds.extent.width,bounds.extent.height}},
                .layerCount = 1,
                .colorAttachmentCount = colorAttachmentCount,
                .pColorAttachments = descriptions.data(),
                .pDepthAttachment = depthAttachment == nullptr? nullptr : &depth,
                .pStencilAttachment = hasStencil ? &depth : nullptr
            };
            vkCmdBeginRendering(_commandBuffer,&render_info);
#ifdef SLAG_DEBUG
            _inRenderPass = true;
#endif

        }

        void IVulkanCommandBuffer::endRendering()
        {
            vkCmdEndRendering(_commandBuffer);
#ifdef SLAG_DEBUG
            _inRenderPass = false;
#endif
        }

        void IVulkanCommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)
        {
            VkViewport viewport{};
            viewport.x = x;
            viewport.y = height+y;
            viewport.width = width;
            viewport.height = -height;
            viewport.minDepth = minDepth;
            viewport.maxDepth = maxDepth;

            vkCmdSetViewport(_commandBuffer,0,1,&viewport);
#if SLAG_DEBUG
            _setViewport = true;
#endif
        }

        void IVulkanCommandBuffer::setScissors(const Rectangle& rect)
        {
            VkRect2D rectangle{.offset{rect.offset.x,rect.offset.y},.extent{rect.extent.width,rect.extent.height}};
            vkCmdSetScissor(_commandBuffer,0,1,&rectangle);
#if SLAG_DEBUG
            _setScissor = true;
#endif
        }

        void IVulkanCommandBuffer::bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            vkCmdBindIndexBuffer(_commandBuffer,vulkanBuffer->vulkanHandle(),offset, indexType == IndexBufferType::UINT_16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        }

        void IVulkanCommandBuffer::bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)
        {
            std::vector<VkBuffer> vulkanBuffers(bufferCount);
            for (auto i = 0; i < bufferCount; i++)
            {
                vulkanBuffers[i] = static_cast<VulkanBuffer*>(buffers[i])->vulkanHandle();
            }
            vkCmdBindVertexBuffers2(_commandBuffer, firstBinding,bufferCount,vulkanBuffers.data(),bufferOffsets,nullptr,strides);
        }

        void IVulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                        uint32_t firstInstance)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            vkCmdDraw(_commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
        }

        void IVulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            int32_t vertexOffset, uint32_t firstInstance)
        {
#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
#endif
            vkCmdDrawIndexed(_commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
        }

        VkCommandBuffer IVulkanCommandBuffer::vulkanHandle() const
        {
            return _commandBuffer;
        }

        void IVulkanCommandBuffer::IVKCBMove(IVulkanCommandBuffer& from)
        {
            std::swap(_commandBuffer, from._commandBuffer);
            std::swap(_commandPool, from._commandPool);
            _graphicsCard = from._graphicsCard;
            _type = from._type;
        }
    } // vulkan
} // slag
