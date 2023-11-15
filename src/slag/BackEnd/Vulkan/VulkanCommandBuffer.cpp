#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanLib.h"
#include "VulkanShader.h"
#include "VulkanVirtualUniformBuffer.h"
#include "VulkanExtensions.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool pool, bool primary,VkQueue submissionQueue, bool destroyImmediately)
        {
            _pool = pool;
            _primary = primary;
            _submissionQueue = submissionQueue;
            this->destroyImmediately = destroyImmediately;
            if(pool)
            {
                //command buffer
                VkCommandBufferAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.pNext = nullptr;

                allocInfo.commandPool = pool;

                allocInfo.commandBufferCount = 1;
                if(primary)
                {
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                }
                else
                {
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                }
                auto result = vkAllocateCommandBuffers(VulkanLib::graphicsCard()->device(), &allocInfo, &_cmdBuffer);
                assert(result == VK_SUCCESS && "failed to allocate command buffer!");
                auto cmdBuffer = _cmdBuffer;
                freeResources = [=]()
                {
                    vkFreeCommandBuffers(VulkanLib::graphicsCard()->device(),pool,1,&cmdBuffer);
                };
            }
        }

        VulkanCommandBuffer::VulkanCommandBuffer(bool primary,VkQueue submissionQueue, uint32_t queueFamily, bool destroyImmediately)
        {
            _primary = primary;
            if(primary)
            {
                _submissionQueue = submissionQueue;
            }
            this->destroyImmediately = destroyImmediately;
            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            commandPoolInfo.queueFamilyIndex = queueFamily;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(VulkanLib::graphicsCard()->device(), &commandPoolInfo, nullptr, &_pool)!=VK_SUCCESS)
            {
                throw std::runtime_error("Unable to initialize local Command Pool");
            }
            if(_pool)
            {
                //command buffer
                VkCommandBufferAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.pNext = nullptr;

                allocInfo.commandPool = _pool;

                allocInfo.commandBufferCount = 1;
                if(primary)
                {
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                }
                else
                {
                    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
                }
                auto result = vkAllocateCommandBuffers(VulkanLib::graphicsCard()->device(), &allocInfo, &_cmdBuffer);
                assert(result == VK_SUCCESS && "failed to allocate command buffer!");
                freeResources = [=]()
                {
                    vkDestroyCommandPool(VulkanLib::graphicsCard()->device(),_pool, nullptr);
                };
            }
        }

        VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&from): Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanCommandBuffer &VulkanCommandBuffer::operator=(VulkanCommandBuffer &&from)
        {
            Resource::operator=(std::move(from));
            move(std::move(from));
            return *this;
        }

        void VulkanCommandBuffer::move(VulkanCommandBuffer &&from)
        {
            std::swap(_cmdBuffer, from._cmdBuffer);
            std::swap(_pool,from._pool);
            std::swap(_primary,from._primary);
            std::swap(_submissionQueue, from._submissionQueue);
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if(_cmdBuffer)
            {
                smartDestroy();
            }
        }

        void* VulkanCommandBuffer::GPUID()
        {
            return _cmdBuffer;
        }

        VkCommandBuffer& VulkanCommandBuffer::vulkanCommandBuffer()
        {
            return _cmdBuffer;
        }

        void VulkanCommandBuffer::reset()
        {
            vkResetCommandBuffer(_cmdBuffer,0);
        }

        CommandBuffer::Level VulkanCommandBuffer::level()
        {
            if(_primary)
            {
                return CommandBuffer::PRIMARY;
            }
            return CommandBuffer::SECONDARY;
        }



        void VulkanCommandBuffer::insertBarriers(const GPUMemoryBarrier *memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier *imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier *bufferBarriers,
                                            size_t bufferBarrierCount, PipelineStage::PipelineStageFlags source, PipelineStage::PipelineStageFlags destination)
        {
            std::vector<VkMemoryBarrier> memBarriers(memoryBarrierCount);
            std::vector<VkImageMemoryBarrier> imBarriers(imageBarrierCount);
            std::vector<VkBufferMemoryBarrier> bufBarriers(bufferBarrierCount);
            //TODO memory barriers
            for(int i=0; i< memoryBarrierCount; i++)
            {

            }
            for(int i=0; i< imageBarrierCount; i++)
            {
                auto curBarrier = imageBarriers+i;
                VulkanTexture* texture = static_cast<VulkanTexture*>(curBarrier->texture);
                imBarriers[i]=VkImageMemoryBarrier
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .srcAccessMask = accessFlagsFromCrossPlatform(curBarrier->requireCachesFinish),
                        .dstAccessMask = accessFlagsFromCrossPlatform(curBarrier->usingCaches),
                        .oldLayout = VulkanTexture::layoutFromCrossPlatform(curBarrier->oldLayout),
                        .newLayout = VulkanTexture::layoutFromCrossPlatform(curBarrier->newLayout),
                        .image = texture->vulkanImage(),
                        .subresourceRange = {
                                .aspectMask = texture->usageVulkan(),
                                .baseMipLevel = 0,
                                .levelCount = texture->mipLevels(),
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                        }
                };

            }
            //TODO buffer barriers
            for(int i=0; i< bufferBarrierCount; i++)
            {

            }
            VkPipelineStageFlags src = pipelineStageFromCrossPlatform(source);
            VkPipelineStageFlags dst = pipelineStageFromCrossPlatform(destination);
            vkCmdPipelineBarrier(_cmdBuffer,src,dst,0,memoryBarrierCount,memBarriers.data(),bufferBarrierCount,bufBarriers.data(),imageBarrierCount,imBarriers.data());
        }


        void VulkanCommandBuffer::begin()
        {
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            auto result =  vkBeginCommandBuffer(_cmdBuffer, &cmdBeginInfo);
            assert(result == VK_SUCCESS && "Unable to begin command buffer");
        }

        void VulkanCommandBuffer::end()
        {
            auto result = vkEndCommandBuffer(_cmdBuffer);
            assert(result==VK_SUCCESS && "Unable to end command buffer");
        }

        void VulkanCommandBuffer::submit(VkSemaphore *waitSemaphores, uint32_t waitCount, VkSemaphore* signalSemaphores, uint32_t signalCount, VkFence fence)
        {
            VkSubmitInfo submit = {};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.pNext = nullptr;

            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            submit.pWaitDstStageMask = &waitStage;

            submit.waitSemaphoreCount = waitCount;
            submit.pWaitSemaphores = waitSemaphores;

            submit.signalSemaphoreCount = signalCount;
            submit.pSignalSemaphores = signalSemaphores;

            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &_cmdBuffer;

            auto result = vkQueueSubmit(_submissionQueue, 1, &submit, fence);
            assert(result == VK_SUCCESS && "Unable to submit command buffer queue");
        }

        void VulkanCommandBuffer::executeSecondaryCommands(CommandBuffer* subBuffer)
        {
            assert(subBuffer->level() == SECONDARY && "subBuffer must be a secondary level command buffer");
            VulkanCommandBuffer* sub = static_cast<VulkanCommandBuffer*>(subBuffer);
            vkCmdExecuteCommands(_cmdBuffer,1,&sub->_cmdBuffer);
        }

        void VulkanCommandBuffer::setTargetFramebuffer(Rectangle bounds, Attachment* colorAttachments, size_t colorCount)
        {
            std::vector<VkRenderingAttachmentInfo> descriptions(colorCount);
            for(auto i=0; i< colorCount; i++)
            {
                auto attachment = colorAttachments[i];
                auto colorTexture = static_cast<VulkanTexture*>(colorAttachments->texture);
                descriptions[i]=VkRenderingAttachmentInfo
                        {
                                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                .imageView = colorTexture->vulkanView(),
                                .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                .clearValue = VulkanTexture::clearValueFromCrossPlatform(attachment.clear),
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
            VkRenderingInfoKHR render_info{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                    .renderArea = {{bounds.offset.x,bounds.offset.y},{bounds.extent.width,bounds.extent.height}},
                    .layerCount = 1,
                    .colorAttachmentCount = static_cast<uint32_t>(colorCount),
                    .pColorAttachments = descriptions.data(),
                    .pDepthAttachment = nullptr,
                    .pStencilAttachment = nullptr
            };
            VulkanExtensions::vkCmdBeginRenderingKHR(_cmdBuffer,&render_info);
        }

        void VulkanCommandBuffer::setTargetFramebuffer(Rectangle bounds, Attachment* colorAttachments, size_t colorCount, Attachment depthAttachment)
        {
            std::vector<VkRenderingAttachmentInfo> descriptions(colorCount);
            for(auto i=0; i< colorCount; i++)
            {
                auto attachment = colorAttachments[i];
                auto colorTexture = static_cast<VulkanTexture*>(colorAttachments->texture);
                descriptions[i]=VkRenderingAttachmentInfo
                        {
                                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                                .imageView = colorTexture->vulkanView(),
                                .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                .clearValue = VulkanTexture::clearValueFromCrossPlatform(attachment.clear),
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

            VkRenderingAttachmentInfo depthRenderingAttachment = {
                    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                    .imageView = static_cast<VulkanTexture*>(depthAttachment.texture)->vulkanView(),
                    .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
                    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                    .clearValue = VulkanTexture::clearValueFromCrossPlatform(depthAttachment.clear),
            };
            if(depthAttachment.clearOnLoad)
            {
                depthRenderingAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            }
            else
            {
                depthRenderingAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            VkRenderingAttachmentInfo* stencil = nullptr;
            if(depthAttachment.texture->usage() | Texture::Usage::STENCIL)
            {
                stencil = &depthRenderingAttachment;
            }

            VkRenderingInfoKHR render_info{
                    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                    .renderArea = {{bounds.offset.x,bounds.offset.y},{bounds.extent.width,bounds.extent.height}},
                    .layerCount = 1,
                    .colorAttachmentCount = static_cast<uint32_t>(colorCount),
                    .pColorAttachments = descriptions.data(),
                    .pDepthAttachment = &depthRenderingAttachment,
                    .pStencilAttachment = stencil
            };
            VulkanExtensions::vkCmdBeginRenderingKHR(_cmdBuffer,&render_info);
        }

        void VulkanCommandBuffer::endTargetFramebuffer()
        {
            VulkanExtensions::vkCmdEndRenderingKHR(_cmdBuffer);
        }

        void VulkanCommandBuffer::bindVertexBuffer(VertexBuffer* vertexBuffer)
        {
            VulkanVertexBuffer* vBuffer = static_cast<VulkanVertexBuffer*>(vertexBuffer);
            VkDeviceSize offset = 0;
            auto buffer = vBuffer->underlyingBuffer();
            vkCmdBindVertexBuffers(_cmdBuffer,0,1,&buffer,&offset);
        }

        void VulkanCommandBuffer::bindIndexBuffer(IndexBuffer* indexBuffer, GraphicsTypes::IndexType indexType)
        {
            VkIndexType itype;
            switch (indexType)
            {
                case GraphicsTypes::IndexType::UINT32:
                    itype = VK_INDEX_TYPE_UINT32;
                    break;
                case GraphicsTypes::IndexType::UINT16:
                    itype = VK_INDEX_TYPE_UINT16;
                    break;

            }
            VulkanIndexBuffer* iBuffer = static_cast<VulkanIndexBuffer*>(indexBuffer);
            vkCmdBindIndexBuffer(_cmdBuffer,iBuffer->underlyingBuffer(),0,itype);
        }

        void VulkanCommandBuffer::bindShader(Shader* shader)
        {
            VulkanShader* vshader = static_cast<VulkanShader*>(shader);
            vkCmdBindPipeline(_cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,vshader->pipeline());
        }

        void VulkanCommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        {
            vkCmdDraw(_cmdBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
        }

        void VulkanCommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, int32_t vertexOffset, uint32_t firstInstance)
        {
            vkCmdDrawIndexed(_cmdBuffer,indexCount,instanceCount,firstVertex,vertexOffset,firstInstance);
        }

        void VulkanCommandBuffer::setViewport(Rectangle bounds)
        {
            VkViewport port {};
            port.x = bounds.offset.x;
            port.y = bounds.offset.y;
            port.width = bounds.extent.width;
            port.height = bounds.extent.height;
            port.maxDepth = 0.0f;
            port.minDepth = 1.0f;
            vkCmdSetViewport(_cmdBuffer,0,1,&port);
        }

        void VulkanCommandBuffer::setScissors(Rectangle bounds)
        {
            VkRect2D area{{bounds.offset.x,bounds.offset.y},{bounds.extent.width,bounds.extent.height}};
            vkCmdSetScissor(_cmdBuffer,0,1,&area);
        }

        void VulkanCommandBuffer::bindUniformSetData(Shader *shader, UniformSetData &data)
        {
            VulkanShader* vulkanShader = static_cast<VulkanShader*>(shader);
            VulkanUniformSet* set = dynamic_cast<VulkanUniformSet *>(data.providingFor());
            VkDescriptorSet descriptorSet = (VkDescriptorSet) data.lowLevelHandle();
            vkCmdBindDescriptorSets(_cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,vulkanShader->layout(),set->index(),1,&descriptorSet,0, nullptr);
        }

        void VulkanCommandBuffer::pushConstants(Shader *shader, PushConstantRange *pushRange, void *data)
        {
            VulkanShader* vulkanShader = static_cast<VulkanShader*>(shader);
            auto range = static_cast<VulkanPushConstantRange*>(pushRange)->range();
            vkCmdPushConstants(_cmdBuffer,vulkanShader->layout(),range.stageFlags,range.offset,range.size, data);
        }

        VkPipelineStageFlags VulkanCommandBuffer::pipelineStageFromCrossPlatform(PipelineStage::PipelineStageFlags flags)
        {
            return static_cast<VkPipelineStageFlags>(flags);
        }

        VkAccessFlags VulkanCommandBuffer::accessFlagsFromCrossPlatform(PipelineAccess::PipeLineAccessFlags access)
        {
            return static_cast<VkPipelineStageFlags>(access);

        }

        CommandBuffer* VulkanCommandBuffer::createSubCommandBuffer()
        {
            return new VulkanCommandBuffer(false,VulkanLib::graphicsCard()->graphicsQueue(),VulkanLib::graphicsCard()->graphicsQueueFamily(), false);
        }

    } // slag
} // vulkan