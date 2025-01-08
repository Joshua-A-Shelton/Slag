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
            VulkanFrame(VkImage backBuffer, uint32_t width, uint32_t height, uint32_t imageIndex, VkImageUsageFlags flags, VulkanSwapchain* from,FrameResources* frameResources);
            ~VulkanFrame()override;
            VulkanFrame(const VulkanFrame&)=delete;
            VulkanFrame& operator=(const VulkanFrame&)=delete;
            VulkanFrame(VulkanFrame&& from);
            VulkanFrame& operator=(VulkanFrame&& from);
            Texture* backBuffer()override;

            VulkanSwapchain* from();
            VkSemaphore commandsFinishedSemaphore();
            friend class VulkanSwapchain;

        private:
            void move(VulkanFrame&& from);
            VulkanSwapchain* _from = nullptr;
            VulkanTexture* _backBuffer = nullptr;
            VkSemaphore _commandsFinished = nullptr;
            uint32_t _imageIndex = 0;

        };

    } // vulkan
} // slag

#endif //SLAG_VULKANFRAME_H
