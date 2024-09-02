#ifndef SLAG_VULKANQUEUE_H
#define SLAG_VULKANQUEUE_H
#include "../../GpuQueue.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanQueue: public GpuQueue
        {
        public:
            VulkanQueue(VkQueue queue, GpuQueue::QueueType type);
            ~VulkanQueue()override;
            VulkanQueue(const VulkanQueue&)=delete;
            VulkanQueue& operator=(const VulkanQueue&)=delete;
            VulkanQueue(VulkanQueue&& from);
            VulkanQueue& operator=(VulkanQueue&& from);

            void submit(CommandBuffer* commands)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)override;
            void submit(CommandBuffer* commands, SemaphoreValue& signalFinished)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished,bool forceDependency)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount)override;

            void submit(CommandBuffer* commands, VkSemaphore imageAcquiredSemaphore, VkSemaphore renderFinishedSemaphore);

            QueueType type()override;
            VkQueue underlyingQueue();
        private:
            VkQueue _queue;
            GpuQueue::QueueType _type;

        };

    } // vulkan
} // slag

#endif //SLAG_VULKANQUEUE_H
