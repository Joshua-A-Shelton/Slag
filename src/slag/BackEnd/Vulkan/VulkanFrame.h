#ifndef SLAG_VULKANFRAME_H
#define SLAG_VULKANFRAME_H
#include "../../Frame.h"
#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanSwapchain;
        class VulkanFrame: public Frame
        {
        public:
            VulkanFrame(VkImage backBuffer, VkImageView view, VulkanSwapchain* from);
            ~VulkanFrame()override;
            VulkanFrame(const VulkanFrame&)=delete;
            VulkanFrame& operator=(const VulkanFrame&)=delete;
            VulkanFrame(VulkanFrame&& from);
            VulkanFrame& operator=(VulkanFrame&& from);
            Texture* backBuffer()override;
            CommandBuffer* commandBuffer()override;
            void present()override;
        private:
            void move(VulkanFrame&& from);
            VulkanSwapchain* _from;
            VulkanTexture* _backBuffer;
            VulkanCommandBuffer* _commandBuffer;

        };

    } // vulkan
} // slag

#endif //SLAG_VULKANFRAME_H
