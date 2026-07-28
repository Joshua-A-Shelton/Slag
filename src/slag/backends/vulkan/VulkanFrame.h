#ifndef SLAG_VULKANFRAME_H
#define SLAG_VULKANFRAME_H
#include <vulkan/vulkan.h>
#include <slag/Slag.h>
#include "VulkanTexture.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapChain;

        class VulkanFrame: public Frame
        {
        public:
            VulkanFrame(uint32_t frameIndex, VkImage image, VkImageView view, uint32_t width, uint32_t height, PixelFormat format, VulkanSwapChain* parentChain);
            ~VulkanFrame()override;
            VulkanFrame(const VulkanFrame&) = delete;
            VulkanFrame& operator=(const VulkanFrame&) = delete;
            VulkanFrame(VulkanFrame&& from) noexcept;
            VulkanFrame& operator=(VulkanFrame&& from) noexcept;
            [[nodiscard]] Texture* renderBuffer()override;
        private:
            void move(VulkanFrame& from);
            uint32_t _frameIndex = 0;
            VulkanTexture* _renderBuffer = nullptr;
            VulkanSwapChain* _parentChain = nullptr;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANFRAME_H