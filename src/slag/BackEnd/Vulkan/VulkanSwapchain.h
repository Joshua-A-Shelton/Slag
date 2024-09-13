#ifndef SLAG_VULKANSWAPCHAIN_H
#define SLAG_VULKANSWAPCHAIN_H
#include "../../Swapchain.h"
#include "VulkanLib.h"
#include "VulkanFrame.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanSwapchain: public Swapchain
        {
        public:
            VulkanSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, PresentMode mode, VulkanizedFormat imageFormat);
            ~VulkanSwapchain()override;
            VulkanSwapchain(const VulkanSwapchain&)=delete;
            VulkanSwapchain& operator=(const VulkanSwapchain&)=delete;
            VulkanSwapchain(VulkanSwapchain&& from);
            VulkanSwapchain& operator=(VulkanSwapchain&& from);
            Frame* next()override;
            Frame* nextIfReady()override;
            Frame* currentFrame()override;
            uint8_t currentFrameIndex()override;
            uint8_t backBuffers()override;
            void backBuffers(uint8_t count)override;
            uint32_t width()override;
            uint32_t height()override;
            void resize(uint32_t width, uint32_t height)override;
            PresentMode presentMode()override;
            void presentMode(PresentMode mode)override;

            VulkanizedFormat imageFormat();

            void rebuild();
            bool needsRebuild();
            VkSwapchainKHR vulkanSwapchain();
            VkSemaphore currentImageAcquiredSemaphore();
            VkFence currentImageAcquiredFence();
            void finishedFrame();
        private:
            void move(VulkanSwapchain&& from);

            static VkSurfaceKHR createNativeSurface(PlatformData platformData);
            VkSurfaceKHR _surface = nullptr;
            VkSwapchainKHR _swapchain = nullptr;
            VulkanizedFormat _imageFormat{};
            uint32_t _width=0;
            uint32_t _height=0;
            uint8_t _backBufferCount=0;
            PresentMode _presentMode = PresentMode::Discard;

            uint32_t _currentFrameIndex=0;
            uint32_t _currentSemaphoreIndex=0;
            bool _needsRebuild=false;
            std::vector<VulkanFrame> _frames;
            std::vector<VkSemaphore> _imageAcquiredSemaphores;
            std::vector<VkFence> _imageAcquiredFences;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANSWAPCHAIN_H
