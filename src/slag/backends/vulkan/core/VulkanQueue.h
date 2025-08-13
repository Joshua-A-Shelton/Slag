#ifndef SLAG_VULKANQUEUE_H
#define SLAG_VULKANQUEUE_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanQueue: public GPUQueue
        {
        public:
            VulkanQueue(VkQueue queue, GPUQueue::QueueType type);
            ~VulkanQueue()override;
            VulkanQueue(const VulkanQueue&) = delete;
            VulkanQueue& operator=(const VulkanQueue&) = delete;
            VulkanQueue(VulkanQueue&& from);
            VulkanQueue& operator=(VulkanQueue&& from);

            virtual QueueType type()override;

            virtual void submit(CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores, size_t signalSemaphoreCount)override;

            virtual void submit(Frame* frame,CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores, size_t signalSemaphoreCount)override;

            VkQueue vulkanHandle();
        private:
            void move(VulkanQueue& from);
            VkQueue _queue=nullptr;
            GPUQueue::QueueType _type=GPUQueue::QueueType::GRAPHICS;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANQUEUE_H
