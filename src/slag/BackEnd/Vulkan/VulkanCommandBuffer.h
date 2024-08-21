#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "../../Resources/Resource.h"
#include "VulkanSemaphore.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanCommandBuffer: public CommandBuffer
        {
        public:
            VulkanCommandBuffer(uint32_t queueFamily);
            ~VulkanCommandBuffer();
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);

            void begin()override;
            void end()override;
            void waitUntilFinished()override;
            bool isFinished()override;
            friend class VulkanQueue;
        private:
            void move(VulkanCommandBuffer&& from);
            void _waitUntilFinished();
            VkCommandBuffer _buffer = nullptr;
            VkCommandPool _pool = nullptr;
            VulkanSemaphore* _finished = nullptr;
            uint32_t _family = 0;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
