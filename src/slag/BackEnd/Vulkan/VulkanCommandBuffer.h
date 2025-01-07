#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "IVulkanCommandBuffer.h"
#include "../../Resources/Resource.h"
#include "../../Resources/ResourceConsumer.h"
#include "VulkanSemaphore.h"
#include "slag/Rectangle.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanTexture;
        class VulkanCommandBuffer: public IVulkanCommandBuffer, resources::ResourceConsumer
        {
        public:
            VulkanCommandBuffer(uint32_t queueFamily);
            ~VulkanCommandBuffer()override;
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);

            void waitUntilFinished()override;
            bool isFinished()override;
            void bindDescriptorPool(DescriptorPool* pool)override;
            friend class VulkanQueue;
            friend class VulkanFrame;

            void begin()override;

        private:
            void move(VulkanCommandBuffer&& from);
            void _waitUntilFinished();
            VulkanSemaphore* _finished = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
