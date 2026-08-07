#ifndef SLAG_VULKANSUBMISSIONQUEUE_H
#define SLAG_VULKANSUBMISSIONQUEUE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;
        class VulkanSubmissionQueue: public SubmissionQueue
        {
        public:
            VulkanSubmissionQueue(VulkanGraphicsCard* graphicsCard, VkQueue queue, QueueType type);
            VulkanSubmissionQueue(const VulkanSubmissionQueue&)=delete;
            VulkanSubmissionQueue& operator=(const VulkanSubmissionQueue&)=delete;
            VulkanSubmissionQueue(VulkanSubmissionQueue&& from) noexcept;
            VulkanSubmissionQueue& operator=(VulkanSubmissionQueue&& from) noexcept;
            ~VulkanSubmissionQueue()override = default;
            [[nodiscard]] QueueType type()const override;
            [[nodiscard]] GraphicsCard* graphicsCard()override;
            void submit(const SubmissionBatch& batch)override;
            void submit(SubmissionBatch* batches, uint32_t batchCount)override;
            VkQueue vulkanHandle()const;
        private:
            void move(VulkanSubmissionQueue& from);
            VkSemaphoreSubmitInfo _waits[5]{VkSemaphoreSubmitInfo{}};
            VkSemaphoreSubmitInfo _signals[5]{VkSemaphoreSubmitInfo{}};
            VkCommandBufferSubmitInfo _commandBuffers[5]{VkCommandBufferSubmitInfo{}};
            VkSemaphoreSubmitInfo* _waitOffsets[5] = {nullptr};
            VkSemaphoreSubmitInfo* _signalOffsets[5] = {nullptr};
            VkCommandBufferSubmitInfo* _commandBufferOffsets[5] = {nullptr};
            VkSubmitInfo2 _submits[5]{VkSubmitInfo2{}};
            VkQueue _queue = nullptr;
            VulkanGraphicsCard* _graphicsCard = nullptr;
            QueueType _type = QueueType::TRANSFER;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSUBMISSIONQUEUE_H
