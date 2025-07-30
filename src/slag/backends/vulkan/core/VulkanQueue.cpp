#include "VulkanQueue.h"

#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"

namespace slag
{
    namespace vulkan
    {
        VulkanQueue::VulkanQueue(VkQueue queue, GPUQueue::QueueType type)
        {
            _queue = queue;
            _type = type;
        }

        VulkanQueue::~VulkanQueue()
        {
        }

        VulkanQueue::VulkanQueue(VulkanQueue&& from)
        {
            move(from);
        }

        VulkanQueue& VulkanQueue::operator=(VulkanQueue&& from)
        {
            move(from);
            return *this;
        }

        GPUQueue::QueueType VulkanQueue::type()
        {
            return _type;
        }

        void VulkanQueue::submit(CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores, size_t signalSemaphoreCount)
        {
            std::vector<VkCommandBufferSubmitInfo> commands(commandBufferCount,{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = nullptr});
            for(size_t i = 0; i < commandBufferCount; i++)
            {
                auto& submitInfo = commands[i];
                VulkanCommandBuffer* commandBuffer = static_cast<VulkanCommandBuffer*>(commandBuffers[i]);
                submitInfo.commandBuffer = commandBuffer->vulkanCommandBufferHandle();
            }
            std::vector<VkSemaphoreSubmitInfo> wait(waitSemaphoreCount, {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT});
            for(size_t i =0; i< waitSemaphoreCount; i++)
            {
                wait[i].semaphore = static_cast<VulkanSemaphore*>(signalSemaphores[i].semaphore)->vulkanSemaphore();
                wait[i].value = signalSemaphores[i].value;
            }
            std::vector<VkSemaphoreSubmitInfo> signal{signalSemaphoreCount,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT}};
            for(size_t i =0; i < signalSemaphoreCount; i++)
            {
                VulkanSemaphore* semaphore = static_cast<VulkanSemaphore*>(signalSemaphores[i].semaphore);
                signal[i].semaphore = semaphore->vulkanSemaphore();
                signal[i].value = signalSemaphores[i].value;
            }
            VkSubmitInfo2 submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = commandBufferCount;
            submitInfo.pCommandBufferInfos = commands.data();
            submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
            submitInfo.pWaitSemaphoreInfos = wait.data();
            submitInfo.signalSemaphoreInfoCount = signalSemaphoreCount;
            submitInfo.pSignalSemaphoreInfos = signal.data();
            vkQueueSubmit2(_queue,1,&submitInfo,nullptr);
        }

        void VulkanQueue::submit(Frame* frame, CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores,size_t signalSemaphoreCount)
        {
            throw std::runtime_error("not implemented");
        }

        void VulkanQueue::move(VulkanQueue& from)
        {
            _queue = from._queue;
            _type = from._type;
        }
    } // vulkan
} // slag
