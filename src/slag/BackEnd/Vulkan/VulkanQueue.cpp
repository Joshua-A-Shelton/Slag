#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanQueue::VulkanQueue(VkQueue queue, GpuQueue::QueueType type)
        {
            _queue = queue;
            _type = type;
        }

        VulkanQueue::~VulkanQueue()
        {
        }

        VulkanQueue::VulkanQueue(VulkanQueue&& from)
        {
            _queue = from._queue;
            _type = from._type;
        }

        VulkanQueue& VulkanQueue::operator=(VulkanQueue&& from)
        {
            _queue = from._queue;
            _type = from._type;
            return *this;
        }

        GpuQueue::QueueType VulkanQueue::type()
        {
            return _type;
        }

        void VulkanQueue::submit(CommandBuffer* commands)
        {
            VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commands);
            //started
            buffer->_finished = new VulkanSemaphore(0, true);

            VkSemaphoreSubmitInfo signalInfo{};
            signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalInfo.semaphore = buffer->_finished->semaphore();
            signalInfo.value = 1;
            signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

            VkCommandBufferSubmitInfo bInfo{};
            bInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            bInfo.commandBuffer = buffer->_buffer;

            VkSubmitInfo2 submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = 1;
            submitInfo.pCommandBufferInfos = &bInfo;
            submitInfo.signalSemaphoreInfoCount = 1;
            submitInfo.pSignalSemaphoreInfos = &signalInfo;
            vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
        }

        void VulkanQueue::submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)
        {
            if(forceDependency)
            {
                VkSemaphore lastSemaphore = nullptr;
                uint32_t waitSemaphoreCount = 0;
                for(size_t i=0; i< bufferCount; i++)
                {
                    VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                    //started
                    buffer->_finished->signal(0);

                    VkSemaphoreSubmitInfo signalInfo{};
                    signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                    signalInfo.semaphore = buffer->_finished->semaphore();
                    signalInfo.value = 1;
                    signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                    VkSemaphoreSubmitInfo waitInfo{};
                    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                    waitInfo.semaphore = lastSemaphore;
                    waitInfo.value = 1;
                    waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                    VkCommandBufferSubmitInfo bInfo{};
                    bInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                    bInfo.commandBuffer = buffer->_buffer;

                    VkSubmitInfo2 submitInfo{};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                    submitInfo.commandBufferInfoCount = 1;
                    submitInfo.pCommandBufferInfos = &bInfo;
                    submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
                    submitInfo.pWaitSemaphoreInfos = &waitInfo;
                    submitInfo.signalSemaphoreInfoCount = 1;
                    submitInfo.pSignalSemaphoreInfos = &signalInfo;


                    vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
                    waitSemaphoreCount = 1;
                    lastSemaphore = buffer->_finished->semaphore();
                }
            }
            else
            {
                std::vector<VkSemaphoreSubmitInfo> signals(bufferCount,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT});
                std::vector<VkCommandBufferSubmitInfo> commands(bufferCount, {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = nullptr});
                for(size_t i=0; i< bufferCount; i++)
                {

                    VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                    //started
                    buffer->_finished->signal(0);
                    signals[i].semaphore = buffer->_finished->semaphore();
                    commands[i].commandBuffer = buffer->_buffer;

                }
                VkSubmitInfo2 submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                submitInfo.commandBufferInfoCount = bufferCount;
                submitInfo.pCommandBufferInfos = commands.data();
                submitInfo.signalSemaphoreInfoCount = bufferCount;
                submitInfo.pSignalSemaphoreInfos = signals.data();
                vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
            }
        }

        void VulkanQueue::submit(CommandBuffer* commands, SemaphoreValue& signalFinished)
        {
            VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commands);
            //started
            buffer->_finished->signal(0);

            VkSemaphoreSubmitInfo signalInfo[2];
            signalInfo[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalInfo[0].semaphore = buffer->_finished->semaphore();
            signalInfo[0].value = 1;
            signalInfo[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

            signalInfo[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            signalInfo[1].semaphore = dynamic_cast<VulkanSemaphore*>(signalFinished.semaphore)->semaphore();
            signalInfo[0].value = signalFinished.value;
            signalInfo[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

            VkCommandBufferSubmitInfo bInfo{};
            bInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
            bInfo.commandBuffer = buffer->_buffer;

            VkSubmitInfo2 submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = 1;
            submitInfo.pCommandBufferInfos = &bInfo;
            submitInfo.signalSemaphoreInfoCount = 2;
            submitInfo.pSignalSemaphoreInfos = signalInfo;
            vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
        }

        void VulkanQueue::submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished, bool forceDependency)
        {
            if(forceDependency)
            {
                VkSemaphore lastSemaphore = nullptr;
                uint32_t waitSemaphoreCount = 0;

                VkSemaphoreSubmitInfo signalInfo[2];
                signalInfo[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                signalInfo[0].semaphore = nullptr;
                signalInfo[0].value = 1;
                signalInfo[0].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                signalInfo[1].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                signalInfo[1].semaphore = dynamic_cast<VulkanSemaphore*>(signalFinished.semaphore)->semaphore();
                signalInfo[1].value = signalFinished.value;
                signalInfo[1].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                for(size_t i=0; i< bufferCount; i++)
                {
                    VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                    //started
                    buffer->_finished->signal(0);

                    signalInfo[0].semaphore = buffer->_finished->semaphore();

                    VkSemaphoreSubmitInfo waitInfo{};
                    waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                    waitInfo.semaphore = lastSemaphore;
                    waitInfo.value = 1;
                    waitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

                    VkCommandBufferSubmitInfo bInfo{};
                    bInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                    bInfo.commandBuffer = buffer->_buffer;

                    VkSubmitInfo2 submitInfo{};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                    submitInfo.commandBufferInfoCount = 1;
                    submitInfo.pCommandBufferInfos = &bInfo;
                    submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
                    submitInfo.pWaitSemaphoreInfos = &waitInfo;
                    if( i == bufferCount-1)
                    {
                        submitInfo.signalSemaphoreInfoCount = 2;
                    }
                    else
                    {
                        submitInfo.signalSemaphoreInfoCount = 1;
                    }

                    submitInfo.pSignalSemaphoreInfos = signalInfo;

                    vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
                    waitSemaphoreCount = 1;
                    lastSemaphore = buffer->_finished->semaphore();
                }
            }
            else
            {
                std::vector<VkSemaphoreSubmitInfo> signals(bufferCount,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT});
                std::vector<VkCommandBufferSubmitInfo> commands(bufferCount, {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = nullptr});
                for(size_t i=0; i< bufferCount; i++)
                {

                    VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                    //started
                    buffer->_finished->signal(0);
                    signals[i].semaphore = buffer->_finished->semaphore();
                    signals[i].value = 1;
                    commands[i].commandBuffer = buffer->_buffer;

                }
                VkSubmitInfo2 submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                submitInfo.commandBufferInfoCount = bufferCount;
                submitInfo.pCommandBufferInfos = commands.data();
                submitInfo.signalSemaphoreInfoCount = bufferCount;
                submitInfo.pSignalSemaphoreInfos = signals.data();
                vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
            }
        }

        void VulkanQueue::submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount)
        {
            std::vector<VkSemaphoreSubmitInfo> signals(bufferCount + signalCount,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT});
            std::vector<VkCommandBufferSubmitInfo> commands(bufferCount, {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = nullptr});
            for(size_t i=0; i< bufferCount; i++)
            {

                VulkanCommandBuffer* buffer = dynamic_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                //started
                buffer->_finished->signal(0);
                signals[i].semaphore = buffer->_finished->semaphore();
                signals[i].value = 1;
                commands[i].commandBuffer = buffer->_buffer;

            }
            for(size_t i= 0; i< signalCount; i++)
            {
                VulkanSemaphore* semaphore = dynamic_cast<VulkanSemaphore*>(signalSemaphores[i].semaphore);
                signals[bufferCount+i].semaphore = semaphore->semaphore();
                signals[bufferCount+i].value = signalSemaphores[i].value;
            }
            std::vector<VkSemaphoreSubmitInfo> wait(waitCount,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT});

            for(size_t i =0; i< waitCount; i++)
            {
                wait[i].semaphore = dynamic_cast<VulkanSemaphore*>(signalSemaphores[i].semaphore)->semaphore();
                wait[i].value = signalSemaphores[i].value;
            }
            VkSubmitInfo2 submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = bufferCount;
            submitInfo.pCommandBufferInfos = commands.data();
            submitInfo.signalSemaphoreInfoCount = bufferCount+signalCount;
            submitInfo.pSignalSemaphoreInfos = signals.data();
            submitInfo.waitSemaphoreInfoCount = waitCount;
            submitInfo.pWaitSemaphoreInfos = wait.data();
            vkQueueSubmit2(_queue,1,&submitInfo, nullptr);
        }

        VkQueue VulkanQueue::underlyingQueue()
        {
            return _queue;
        }
    } // vulkan
} // slag