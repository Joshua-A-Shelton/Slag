#include "VulkanSubmissionQueue.h"

#include <memory>

#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSemaphore.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSubmissionQueue::VulkanSubmissionQueue(VulkanGraphicsCard* graphicsCard, VkQueue queue, QueueType type)
        {
            _graphicsCard = graphicsCard;
            _queue = queue;
            _type = type;
        }

        VulkanSubmissionQueue::VulkanSubmissionQueue(VulkanSubmissionQueue&& from) noexcept: _queue(nullptr), _type(QueueType::TRANSFER), _graphicsCard(nullptr)
        {
            move(from);
        }

        VulkanSubmissionQueue& VulkanSubmissionQueue::operator=(VulkanSubmissionQueue&& from) noexcept
        {
            move(from);
            return *this;
        }

        QueueType VulkanSubmissionQueue::type() const
        {
            return _type;
        }

        GraphicsCard* VulkanSubmissionQueue::graphicsCard()
        {
            return _graphicsCard;
        }

        //FIXME: there's a ton of allocations happening in this function, it really shouldn't be this bad
        void VulkanSubmissionQueue::submit(SubmissionBatch* batches, uint32_t batchCount)
        {
            SLAG_ASSERT(batchCount > 0 && "At least one batch must be submitted");
            SLAG_ASSERT(batches != nullptr && "Parameter \"batches\" must not be nullptr");

            std::vector<VkSubmitInfo2> submit(batchCount,{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2});
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> waits(batchCount);
            std::vector<std::unique_ptr<std::vector<VkSemaphoreSubmitInfo>>> signals(batchCount);
            std::vector<std::unique_ptr<std::vector<VkCommandBufferSubmitInfo>>> commandBuffers(batchCount);
            for (auto i = 0; i < batchCount; i++)
            {
                auto& submitInfo = submit[i];
                auto& submissionDatum = batches[i];
                auto& wait = waits[i];
                auto& signal = signals[i];
                auto& buffers = commandBuffers[i];

                wait = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.waitSemaphoreCount);
                for (auto j = 0; j < submissionDatum.waitSemaphoreCount; j++)
                {
                    auto& waitSemaphore = submissionDatum.waitSemaphores[j];
                    auto semaphore = static_cast<VulkanSemaphore*>(waitSemaphore.semaphore);
                    (*wait)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanHandle(),.value = waitSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                signal = std::make_unique<std::vector<VkSemaphoreSubmitInfo>>(submissionDatum.signalSemaphoreCount);
                for (auto j = 0; j < submissionDatum.signalSemaphoreCount; j++)
                {
                    auto& signalSemaphore = submissionDatum.signalSemaphores[j];
                    auto semaphore = static_cast<VulkanSemaphore*>(signalSemaphore.semaphore);
                    (*signal)[j] = VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,.semaphore = semaphore->vulkanHandle(),.value = signalSemaphore.value,.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
                }
                buffers = std::make_unique<std::vector<VkCommandBufferSubmitInfo>>(submissionDatum.commandBufferCount);
                for (auto j = 0; j < submissionDatum.commandBufferCount; j++)
                {
                    auto commandBuffer = static_cast<VulkanCommandBuffer*>(submissionDatum.commandBuffers[j]);
                    SLAG_ASSERT(QueueTypeSupportsCommands(_type,commandBuffer->type()) && "Queue cannot process command buffer outside it's capabilities");
                    (*buffers)[j] = VkCommandBufferSubmitInfo{.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = commandBuffer->vulkanHandle()};
                }


                submitInfo.waitSemaphoreInfoCount = submissionDatum.waitSemaphoreCount;
                submitInfo.pWaitSemaphoreInfos = wait->data();
                submitInfo.commandBufferInfoCount = submissionDatum.commandBufferCount;
                submitInfo.pCommandBufferInfos = buffers->data();
                submitInfo.signalSemaphoreInfoCount = submissionDatum.signalSemaphoreCount;
                submitInfo.pSignalSemaphoreInfos = signal->data();
            }
            vkQueueSubmit2(_queue,batchCount,submit.data(),nullptr);
        }

        VkQueue VulkanSubmissionQueue::vulkanHandle() const
        {
            return _queue;
        }

        void VulkanSubmissionQueue::move(VulkanSubmissionQueue& from)
        {
            _queue = from._queue;
            _graphicsCard = from._graphicsCard;
            _type = from._type;
        }
    } // vulkan
} // slag
