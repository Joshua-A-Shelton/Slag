#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(VkCommandPool pool)
        {
            _pool = pool;
            if(pool)
            {
                //command buffer
                VkCommandBufferAllocateInfo allocInfo = {};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.pNext = nullptr;

                allocInfo.commandPool = pool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;
                auto result = vkAllocateCommandBuffers(VulkanLib::graphicsCard()->device(), &allocInfo, &_cmdBuffer);
                assert(result == VK_SUCCESS && "failed to allocate command buffer!");
            }
        }

        VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&from)
        {
            move(std::move(from));
        }

        VulkanCommandBuffer &VulkanCommandBuffer::operator=(VulkanCommandBuffer &&from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanCommandBuffer::move(VulkanCommandBuffer &&from)
        {
            std::swap(_cmdBuffer, from._cmdBuffer);
            std::swap(_pool,from._pool);
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if(_cmdBuffer)
            {
                vkFreeCommandBuffers(VulkanLib::graphicsCard()->device(),_pool,1,&_cmdBuffer);
            }
        }

        VkCommandBuffer& VulkanCommandBuffer::vulkanCommandBuffer()
        {
            return _cmdBuffer;
        }

        void VulkanCommandBuffer::reset()
        {
            vkResetCommandBuffer(_cmdBuffer,0);
        }

        void VulkanCommandBuffer::insertMemoryBarrier(const GPUMemoryBarrier &barrier, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(&barrier,1, nullptr,0, nullptr,0,source,destination);
        }

        void VulkanCommandBuffer::insertMemoryBarriers(const GPUMemoryBarrier *barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(barriers,count, nullptr,0, nullptr,0,source,destination);
        }

        void VulkanCommandBuffer::insertImageBarrier(const ImageMemoryBarrier &barrier, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(nullptr,0, &barrier,1, nullptr,0,source,destination);
        }

        void VulkanCommandBuffer::insertImageBarriers(const ImageMemoryBarrier *barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(nullptr,0, barriers,count, nullptr,0,source,destination);
        }

        void VulkanCommandBuffer::insertBufferBarrier(const BufferMemoryBarrier &barrier, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(nullptr,0, nullptr,0, &barrier,1,source,destination);
        }

        void VulkanCommandBuffer::insertBufferBarriers(const BufferMemoryBarrier *barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)
        {
            insertBarriers(nullptr,0, nullptr,0, barriers,count,source,destination);
        }

        void
        VulkanCommandBuffer::insertBarriers(const GPUMemoryBarrier *memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier *imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier *bufferBarriers,
                                            size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)
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
                        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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

        VkPipelineStageFlags VulkanCommandBuffer::pipelineStageFromCrossPlatform(PipelineStageFlags flags)
        {
            return static_cast<VkPipelineStageFlags>(flags);
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

        void VulkanCommandBuffer::submit(VkSemaphore *waitSemaphores, uint32_t waitCount, VkSemaphore* signalSemaphores, uint32_t signalCount, VkQueue submitQueue, VkFence fence)
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

            auto result = vkQueueSubmit(VulkanLib::graphicsCard()->graphicsQueue(), 1, &submit, fence);
            assert(result == VK_SUCCESS && "Unable to submit render queue");
        }


    } // slag
} // vulkan