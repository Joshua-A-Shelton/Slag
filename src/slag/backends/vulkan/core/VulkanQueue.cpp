#include "VulkanQueue.h"

#include "VulkanCommandBuffer.h"
#include "VulkanFrame.h"
#include "VulkanSemaphore.h"
#include "VulkanSwapChain.h"

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
            std::vector<VkCommandBufferSubmitInfo> commands(commandBufferCount,{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = nullptr});
            VulkanFrame* vulkanFrame = static_cast<VulkanFrame*>(frame);
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
            std::vector<VkSemaphoreSubmitInfo> signal{signalSemaphoreCount+1,{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = nullptr,.value =1, .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT}};
            for(size_t i =0; i < signalSemaphoreCount; i++)
            {
                VulkanSemaphore* semaphore = static_cast<VulkanSemaphore*>(signalSemaphores[i].semaphore);
                signal[i].semaphore = semaphore->vulkanSemaphore();
                signal[i].value = signalSemaphores[i].value;
            }

            //set the commands complete semaphore
            signal[signalSemaphoreCount].semaphore = vulkanFrame->commandsCompleteSemaphore();


            VkSubmitInfo2 submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
            submitInfo.commandBufferInfoCount = commandBufferCount;
            submitInfo.pCommandBufferInfos = commands.data();
            submitInfo.waitSemaphoreInfoCount = waitSemaphoreCount;
            submitInfo.pWaitSemaphoreInfos = wait.data();
            submitInfo.signalSemaphoreInfoCount = signal.size();
            submitInfo.pSignalSemaphoreInfos = signal.data();
            vkQueueSubmit2(_queue,1,&submitInfo,vulkanFrame->commandsCompleteFence());

            VkSemaphore waitPresentSemaphores[2]{vulkanFrame->commandsCompleteSemaphore(),vulkanFrame->imageAcquiredSemaphore()};
            auto currentImageIndex = vulkanFrame->parentSwapChain()->currentImageIndex();
            auto swapChain = vulkanFrame->parentSwapChain()->vulkanHandle();
            VkResult presentSuccess = VK_SUCCESS;
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 2;
            presentInfo.pWaitSemaphores = waitPresentSemaphores;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &currentImageIndex;
            presentInfo.pResults = &presentSuccess;


            auto ImageAcquiredFence = vulkanFrame->imageAquiredFence();
            VkSwapchainPresentFenceInfoEXT fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
            fenceInfo.swapchainCount = 1;
            fenceInfo.pFences = &ImageAcquiredFence;

            presentInfo.pNext = &fenceInfo;

            vkQueuePresentKHR(_queue,&presentInfo);
            vulkanFrame->parentSwapChain()->advance();

            if (presentSuccess == VK_ERROR_OUT_OF_DATE_KHR || presentSuccess == VK_SUBOPTIMAL_KHR)
            {
                vulkanFrame->parentSwapChain()->invalidate();
            }
        }

        VkQueue VulkanQueue::vulkanHandle()
        {
            return _queue;
        }

        void VulkanQueue::move(VulkanQueue& from)
        {
            _queue = from._queue;
            _type = from._type;
        }
    } // vulkan
} // slag
