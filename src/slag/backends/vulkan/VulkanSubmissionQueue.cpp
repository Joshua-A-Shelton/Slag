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
