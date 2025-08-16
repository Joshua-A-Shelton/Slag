#include "VulkanQueue.h"

#include "VulkanCommandBuffer.h"
#include "VulkanFrame.h"
#include "VulkanSemaphore.h"
#include "VulkanSwapChain.h"
#include "slag/utilities/SLAG_ASSERT.h"

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

        void VulkanQueue::submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount)
        {
            SLAG_ASSERT(submissionDataCount > 0 && submissionData!= nullptr && "Cannot submit empty data to queue");

            std::vector<VkSubmitInfo2> submit(submissionDataCount,{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2});
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> waits(submissionDataCount);
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> signals(submissionDataCount);
            std::vector<std::unique_ptr<std::vector<VkCommandBufferSubmitInfo>>> commandBuffers(submissionDataCount);
            for (auto i = 0; i < submissionDataCount; i++)
            {
                auto& submitInfo = submit[i];
                auto& submissionDatum = submissionData[i];
                auto& wait = waits[i];
                auto& signal = signals[i];
                auto& buffers = commandBuffers[i];

                wait = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.waitSemaphoreCount);
                for (auto j = 0; j < submissionDatum.waitSemaphoreCount; j++)
                {
                    auto& waitSemaphore = submissionDatum.waitSemaphores[j];
                    auto semaphore = static_cast<VulkanSemaphore*>(waitSemaphore.semaphore);
                    (*wait)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanSemaphore(),.value = waitSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                signal = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.signalSemaphoreCount);
                for (auto j = 0; j < submissionDatum.signalSemaphoreCount; j++)
                {
                    auto& signalSemaphore = submissionDatum.signalSemaphores[j];
                    auto semaphore = static_cast<VulkanSemaphore*>(signalSemaphore.semaphore);
                    (*signal)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanSemaphore(),.value = signalSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                buffers = std::make_unique<std::vector<VkCommandBufferSubmitInfo>>(submissionDatum.commandBufferCount);
                for (auto j = 0; j < submissionDatum.commandBufferCount; j++)
                {
                    auto commandBuffer = static_cast<VulkanCommandBuffer*>(submissionDatum.commandBuffers[j]);
                    SLAG_ASSERT(GPUQueue::canProcessCommands(_type,commandBuffer->commandType()) && "Queue cannot process command buffer outside it's capabilities");
                    (*buffers)[j] = VkCommandBufferSubmitInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = commandBuffer->vulkanCommandBufferHandle()};
                }


                submitInfo.waitSemaphoreInfoCount = submissionDatum.waitSemaphoreCount;
                submitInfo.pWaitSemaphoreInfos = wait->data();
                submitInfo.commandBufferInfoCount = submissionDatum.commandBufferCount;
                submitInfo.pCommandBufferInfos = buffers->data();
                submitInfo.signalSemaphoreInfoCount = submissionDatum.signalSemaphoreCount;
                submitInfo.pSignalSemaphoreInfos = signal->data();
            }
            vkQueueSubmit2(_queue,submissionDataCount,submit.data(),nullptr);
        }

        void VulkanQueue::submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount, Frame* frame)
        {
            SLAG_ASSERT(_type == QueueType::GRAPHICS && "frame submissions can only be done on a graphics queue");

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            submitGeneral(submissionData, submissionDataCount, frame);

#else
            submitDiscreet(submissionData, submissionDataCount, frame);
#endif

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

        void VulkanQueue::submitGeneral(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount, Frame* frame)
        {
            SLAG_ASSERT(submissionDataCount > 0 && submissionData!= nullptr && frame != nullptr);
            auto vulkanFrame = static_cast<VulkanFrame*>(frame);

            std::vector<VkSubmitInfo2> submit(submissionDataCount+2,{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2});
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> waits(submissionDataCount+2);
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> signals(submissionDataCount+2);
            std::vector<std::unique_ptr<std::vector<VkCommandBufferSubmitInfo>>> commandBuffers(submissionDataCount+2);
            //fill out the general submits, but leave a space at the beginning and end for the image transitions
            for (auto i = 1; i < submissionDataCount+1; i++)
            {
                auto& submitInfo = submit[i];
                auto& submissionDatum = submissionData[i-1];
                auto& wait = waits[i];
                auto& signal = signals[i];
                auto& buffers = commandBuffers[i];

                int offset = 0;
                // if it's the first of the original command buffers, add an additional wait semaphore slot
                if (i==1)
                {
                    wait = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.waitSemaphoreCount+1);
                    offset = 1;
                }
                else
                {
                    wait= std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.waitSemaphoreCount);
                }
                for (auto j = 0+offset; j < submissionDatum.waitSemaphoreCount+offset; j++)
                {
                    auto& waitSemaphore = submissionDatum.waitSemaphores[j-offset];
                    auto semaphore = static_cast<VulkanSemaphore*>(waitSemaphore.semaphore);
                    (*wait)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanSemaphore(),.value = waitSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                //if it's the first of the original command buffers, add a wait for swapchain image transfer to finish before execution
                if (i==1)
                {
                    (*wait)[0] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = vulkanFrame->backBufferToGeneralSemaphore(),.value = 1,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }

                //if it's the last of the original command buffers, add an additional signal slot
                if (i==submissionDataCount)
                {
                    signal = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.signalSemaphoreCount+1);
                }
                else
                {
                    signal = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.signalSemaphoreCount);
                }
                for (auto j = 0; j < submissionDatum.signalSemaphoreCount; j++)
                {
                    auto& signalSemaphore = submissionDatum.signalSemaphores[j];
                    auto semaphore = static_cast<VulkanSemaphore*>(signalSemaphore.semaphore);
                    (*signal)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanSemaphore(),.value = signalSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                //if it's the last of the original command buffers, add additional signal
                if (i==submissionDataCount)
                {
                    (*signal)[submissionDatum.signalSemaphoreCount] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = vulkanFrame->submittedCompleteSemaphore(),.value = 1,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }

                buffers = std::make_unique<std::vector<VkCommandBufferSubmitInfo>>(submissionDatum.commandBufferCount);
                for (auto j = 0; j < submissionDatum.commandBufferCount; j++)
                {
                    auto commandBuffer = static_cast<VulkanCommandBuffer*>(submissionDatum.commandBuffers[j]);
                    SLAG_ASSERT(GPUQueue::canProcessCommands(_type,commandBuffer->commandType()) && "Queue cannot process command buffer outside it's capabilities");
                    (*buffers)[j] = VkCommandBufferSubmitInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = commandBuffer->vulkanCommandBufferHandle()};
                }


                submitInfo.waitSemaphoreInfoCount = wait->size();
                submitInfo.pWaitSemaphoreInfos = wait->data();
                submitInfo.commandBufferInfoCount = submissionDatum.commandBufferCount;
                submitInfo.pCommandBufferInfos = buffers->data();
                submitInfo.signalSemaphoreInfoCount = signal->size();
                submitInfo.pSignalSemaphoreInfos = signal->data();
            }

            //setup transition into general
            auto& transitionIntoGeneral = submit[0];
            VkSemaphoreSubmitInfo signalGeneral{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = vulkanFrame->backBufferToGeneralSemaphore(),.value = 1};
            auto commandBufferGeneral = vulkanFrame->backBufferToGeneral();
            VkCommandBufferSubmitInfo buffersGeneralSubmit{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,.commandBuffer = commandBufferGeneral->vulkanCommandBufferHandle()};
            auto backBuffer = vulkanFrame->backBuffer();


            commandBufferGeneral->begin();
            commandBufferGeneral->transitionToLayout(backBuffer,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL,VK_ACCESS_2_NONE,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            commandBufferGeneral->end();


            transitionIntoGeneral.pWaitSemaphoreInfos = nullptr;
            transitionIntoGeneral.waitSemaphoreInfoCount = 0;
            transitionIntoGeneral.pCommandBufferInfos = &buffersGeneralSubmit;
            transitionIntoGeneral.commandBufferInfoCount = 1;
            transitionIntoGeneral.pSignalSemaphoreInfos = &signalGeneral;
            transitionIntoGeneral.signalSemaphoreInfoCount = 1;


            //setup transition into present

            auto& transitionIntoPresent= submit[submit.size()-1];
            VkSemaphoreSubmitInfo waitPresent{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = vulkanFrame->submittedCompleteSemaphore(),.value = 1};
            VkSemaphoreSubmitInfo signalPresent{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = vulkanFrame->commandsCompleteSemaphore(),.value = 1};
            auto commandBufferPresent = vulkanFrame->backBufferToPresent();
            VkCommandBufferSubmitInfo buffersPresent {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,.commandBuffer = commandBufferPresent->vulkanCommandBufferHandle()};


            commandBufferPresent->begin();
            commandBufferPresent->transitionToLayout(backBuffer,VK_IMAGE_LAYOUT_GENERAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT,VK_ACCESS_2_NONE,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
            commandBufferPresent->end();


            transitionIntoPresent.pWaitSemaphoreInfos = &waitPresent;
            transitionIntoPresent.waitSemaphoreInfoCount = 1;
            transitionIntoPresent.pCommandBufferInfos = &buffersPresent;
            transitionIntoPresent.commandBufferInfoCount = 1;
            transitionIntoPresent.pSignalSemaphoreInfos = &signalPresent;
            transitionIntoPresent.signalSemaphoreInfoCount = 1;

            vkQueueSubmit2(_queue,submit.size(),submit.data(),vulkanFrame->commandsCompleteFence());

            //Present image
            VkSemaphore waitPresentSemaphores[]{vulkanFrame->commandsCompleteSemaphore(),vulkanFrame->imageAcquiredSemaphore()};
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

            auto frameFinished = vulkanFrame->imageAcquiredFence();
            VkSwapchainPresentFenceInfoEXT fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
            fenceInfo.swapchainCount = 1;
            fenceInfo.pFences = &frameFinished;

            presentInfo.pNext = &fenceInfo;

            vkQueuePresentKHR(_queue,&presentInfo);
            vulkanFrame->parentSwapChain()->advance();

            if (presentSuccess == VK_ERROR_OUT_OF_DATE_KHR || presentSuccess == VK_SUBOPTIMAL_KHR)
            {
                vulkanFrame->parentSwapChain()->invalidate();
            }

        }

        void VulkanQueue::submitDiscreet(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount,Frame* frame)
        {
            throw std::runtime_error(__FUNCTION__);
        }
    } // vulkan
} // slag
