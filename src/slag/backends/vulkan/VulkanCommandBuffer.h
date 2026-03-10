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
            VulkanCommandBuffer(VulkanGraphicsCard* graphicsCard,QueueType type);
            ~VulkanCommandBuffer()override;
            VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from) noexcept;
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from) noexcept;

        private:
            void move(VulkanCommandBuffer& from);
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
