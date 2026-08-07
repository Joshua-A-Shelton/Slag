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

        void VulkanSubmissionQueue::submit(const SubmissionBatch& batch)
        {

            VkSubmitInfo2 submitInfo {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2};

            VkSemaphoreSubmitInfo* waits = _waits;
            std::vector<VkSemaphoreSubmitInfo> waitsDynamic(0);
            if (batch.waitSemaphoreCount > _countof(_waits))
            {
                waitsDynamic.resize(batch.waitSemaphoreCount);
                waits = waitsDynamic.data();
            }
            VkSemaphoreSubmitInfo* signals = _signals;
            std::vector<VkSemaphoreSubmitInfo> signalsDynamic(0);
            if (batch.signalSemaphoreCount > _countof(_signals))
            {
                signalsDynamic.resize(batch.signalSemaphoreCount);
                signals = signalsDynamic.data();
            }
            VkCommandBufferSubmitInfo* commandBuffers = _commandBuffers;
            std::vector<VkCommandBufferSubmitInfo> commandBuffersDynamic(0);
            if (batch.commandBufferCount > _countof(_commandBuffers))
            {
                commandBuffersDynamic.resize(batch.commandBufferCount);
                commandBuffers = commandBuffersDynamic.data();
            }

            for (auto i = 0u; i < batch.waitSemaphoreCount; i++)
            {
                auto& semaphore = batch.waitSemaphores[i];
                auto& semaphoreInfo = waits[i];
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                semaphoreInfo.pNext = nullptr;
                semaphoreInfo.semaphore = static_cast<VulkanSemaphore*>(semaphore.semaphore)->vulkanHandle();
                semaphoreInfo.value = semaphore.value;
                semaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            }

            for (auto i = 0u; i < batch.commandBufferCount; i++)
            {
                auto& commandBuffer = batch.commandBuffers[i];
                SLAG_ASSERT(QueueTypeSupportsCommands(_type,commandBuffer->type()) && "Queue cannot process command buffer outside it's capabilities");

                auto& commandBufferInfo = commandBuffers[i];
                commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                commandBufferInfo.pNext = nullptr;
                commandBufferInfo.commandBuffer = static_cast<VulkanCommandBuffer*>(commandBuffer)->vulkanHandle();
                commandBufferInfo.deviceMask = 0;
            }

            for (auto i = 0u; i < batch.signalSemaphoreCount; i++)
            {
                auto& semaphore = batch.signalSemaphores[i];
                auto& semaphoreInfo = signals[i];
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                semaphoreInfo.pNext = nullptr;
                semaphoreInfo.semaphore = static_cast<VulkanSemaphore*>(semaphore.semaphore)->vulkanHandle();
                semaphoreInfo.value = semaphore.value;
                semaphoreInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
            }

            submitInfo.waitSemaphoreInfoCount = batch.waitSemaphoreCount;
            submitInfo.pWaitSemaphoreInfos = waits;
            submitInfo.commandBufferInfoCount = batch.commandBufferCount;
            submitInfo.pCommandBufferInfos = commandBuffers;
            submitInfo.signalSemaphoreInfoCount = batch.signalSemaphoreCount;
            submitInfo.pSignalSemaphoreInfos = signals;

            vkQueueSubmit2(_queue,1,&submitInfo,nullptr);
        }

        void VulkanSubmissionQueue::submit(SubmissionBatch* batches, uint32_t batchCount)
        {
            VkSubmitInfo2* submissions = _submits;
            VkSemaphoreSubmitInfo** waitOffsets = _waitOffsets;
            VkSemaphoreSubmitInfo** signalOffsets = _signalOffsets;
            VkCommandBufferSubmitInfo** commandBufferOffsets = _commandBufferOffsets;
            std::vector<VkSubmitInfo2> submissionsDynamic(0);
            std::vector<VkSemaphoreSubmitInfo*> waitOffsetsDynamic(0);
            std::vector<VkSemaphoreSubmitInfo*> signalOffsetsDynamic(0);
            std::vector<VkCommandBufferSubmitInfo*> commandBufferOffsetsDynamic(0);
            if (batchCount > _countof(_submits))
            {
                submissionsDynamic.resize(batchCount);
                submissions = submissionsDynamic.data();
                waitOffsetsDynamic.resize(batchCount);
                waitOffsets = waitOffsetsDynamic.data();
                signalOffsetsDynamic.resize(batchCount);
                signalOffsets = signalOffsetsDynamic.data();
                commandBufferOffsetsDynamic.resize(batchCount);
                commandBufferOffsets = commandBufferOffsetsDynamic.data();
            }

            uint32_t totalWaitSemaphoreCount = 0;
            uint32_t totalSignalSemaphoreCount = 0;
            uint32_t totalCommandBufferCount = 0;
            for (auto i = 0u; i < batchCount; i++)
            {
                if (batches[i].waitSemaphoreCount ==0)
                {
                    waitOffsets[i] = nullptr;
                }
                else
                {
                    waitOffsets[i] = _waits + totalWaitSemaphoreCount;
                }
                totalWaitSemaphoreCount += batches[i].waitSemaphoreCount;

                if (batches[i].signalSemaphoreCount == 0)
                {
                    signalOffsets[i] = nullptr;
                }
                else
                {
                    signalOffsets[i] = _signals + totalSignalSemaphoreCount;
                }
                totalSignalSemaphoreCount += batches[i].signalSemaphoreCount;

                if (batches[i].commandBufferCount == 0)
                {
                    commandBufferOffsets[i] = nullptr;
                }
                else
                {
                    commandBufferOffsets[i] = _commandBuffers + totalCommandBufferCount;
                }
                totalCommandBufferCount += batches[i].commandBufferCount;
            }

            VkSemaphoreSubmitInfo* waits = _waits;
            std::vector<VkSemaphoreSubmitInfo> waitsDynamic(0);
            if (totalWaitSemaphoreCount > _countof(_waits))
            {
                waitsDynamic.resize(totalWaitSemaphoreCount);
                waits = waitsDynamic.data();
            }
            VkSemaphoreSubmitInfo* signals = _signals;
            std::vector<VkSemaphoreSubmitInfo> signalsDynamic(0);
            if (totalSignalSemaphoreCount > _countof(_signals))
            {
                signalsDynamic.resize(totalSignalSemaphoreCount);
                signals = signalsDynamic.data();
            }
            VkCommandBufferSubmitInfo* commandBuffers = _commandBuffers;
            std::vector<VkCommandBufferSubmitInfo> commandBuffersDynamic(0);
            if (totalCommandBufferCount > _countof(_commandBuffers))
            {
                commandBuffersDynamic.resize(totalCommandBufferCount);
                commandBuffers = commandBuffersDynamic.data();
            }

            uint32_t waitSemaphoreIndex = 0;
            uint32_t signalSemaphoreIndex = 0;
            uint32_t commandBufferIndex = 0;
            for (auto i = 0u; i < batchCount; i++)
            {
                auto& batch = batches[i];
                auto& submission = submissions[i];
                submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
                submission.pNext = nullptr;
                submission.waitSemaphoreInfoCount = batch.waitSemaphoreCount;
                submission.pWaitSemaphoreInfos = waitOffsets[i];
                submission.commandBufferInfoCount = batch.commandBufferCount;
                submission.pCommandBufferInfos = commandBufferOffsets[i];
                submission.signalSemaphoreInfoCount = batch.signalSemaphoreCount;
                submission.pSignalSemaphoreInfos = signalOffsets[i];

                for (auto j = 0u; j < batch.waitSemaphoreCount; j++)
                {
                    auto& semaphore = batch.waitSemaphores[j];
                    auto& semaphoreInfo = waits[waitSemaphoreIndex];
                    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                    semaphoreInfo.pNext = nullptr;
                    semaphoreInfo.semaphore = static_cast<VulkanSemaphore*>(semaphore.semaphore)->vulkanHandle();
                    semaphoreInfo.value = semaphore.value;
                    waitSemaphoreIndex++;
                }
                for (auto j = 0u; j < batch.commandBufferCount; j++)
                {
                    auto& commandBuffer = batch.commandBuffers[j];
                    SLAG_ASSERT(QueueTypeSupportsCommands(_type,commandBuffer->type()) && "Queue cannot process command buffer outside it's capabilities");
                    auto& commandBufferInfo = commandBuffers[commandBufferIndex];
                    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
                    commandBufferInfo.pNext = nullptr;
                    commandBufferInfo.commandBuffer = static_cast<VulkanCommandBuffer*>(commandBuffer)->vulkanHandle();
                    commandBufferInfo.deviceMask = 0;
                    commandBufferIndex++;
                }
                for (auto j = 0u; j < batch.signalSemaphoreCount; j++)
                {
                    auto& semaphore = batch.signalSemaphores[j];
                    auto& semaphoreInfo = signals[signalSemaphoreIndex];
                    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
                    semaphoreInfo.pNext = nullptr;
                    semaphoreInfo.semaphore = static_cast<VulkanSemaphore*>(semaphore.semaphore)->vulkanHandle();
                    semaphoreInfo.value = semaphore.value;
                    signalSemaphoreIndex++;
                }
            }

            vkQueueSubmit2(_queue,batchCount,&submissions[0],nullptr);

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
