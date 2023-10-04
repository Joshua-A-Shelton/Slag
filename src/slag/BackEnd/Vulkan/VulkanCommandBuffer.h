#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanCommandBuffer: public CommandBuffer
        {
        private:
            VkCommandBuffer _cmdBuffer = nullptr;
            void move(VulkanCommandBuffer&& from);
        public:
            VulkanCommandBuffer(VkCommandBuffer cmdBuffer);
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);
            ~VulkanCommandBuffer()override;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANCOMMANDBUFFER_H