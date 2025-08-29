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
            VulkanCommandBuffer(GPUQueue::QueueType type);
            virtual ~VulkanCommandBuffer()override;
            VulkanCommandBuffer(VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);

            virtual void bindDescriptorPool(DescriptorPool* pool) override;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
