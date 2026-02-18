#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include <slag/Slag.h>
#include "IVulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanCommandBuffer: public IVulkanCommandBuffer
        {
        public:
            VulkanCommandBuffer(VulkanGraphicsCard* graphicsCard,QueueType type, CommandBufferLevel level);
            ~VulkanCommandBuffer()override;
            VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);

        private:
            void move(VulkanCommandBuffer& from);
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
