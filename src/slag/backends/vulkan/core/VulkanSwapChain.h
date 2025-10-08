#ifndef SLAG_VULKANSWAPCHAIN_H
#define SLAG_VULKANSWAPCHAIN_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "VulkanFrame.h"
#include "slag/core/Pixels.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapChain: public SwapChain
        {
        public:

            VulkanSwapChain(PlatformData platformData, uint32_t width, uint32_t height, PresentMode presentMode, uint8_t frameCount,Pixels::Format format, AlphaCompositing compositing, FrameResources* (*createResourceFunction)(uint8_t frameIndex, SwapChain* inChain), void (*swapchainRebuiltFunction)(SwapChain* swapChain));
            ~VulkanSwapChain()override;
            VulkanSwapChain(VulkanSwapChain const&) = delete;
            VulkanSwapChain& operator=(VulkanSwapChain const&) = delete;
            VulkanSwapChain(VulkanSwapChain&& from);
            VulkanSwapChain& operator=(VulkanSwapChain&& from);

            virtual Frame* next()override;
            virtual Frame* nextIfReady()override;
            virtual Frame* currentFrame()override;
            virtual uint8_t currentFrameIndex()override;
            virtual Pixels::Format backBufferFormat()override;
            virtual void backBufferFormat(Pixels::Format newFormat)override;
            virtual uint32_t backBufferWidth()override;
            virtual uint32_t backBufferHeight()override;
            virtual void backBufferSize(uint32_t newWidth, uint32_t newHeight)override;
            virtual PresentMode presentMode()override;
            virtual uint8_t frameCount()override;
            virtual void presentMode(PresentMode newMode, uint8_t frameCount)override;
            virtual AlphaCompositing alphaCompositing()override;
            virtual void alphaComposition(AlphaCompositing newCompositing)override;
            virtual void setProperties(uint32_t newWidth, uint32_t newHeight, uint8_t frameCount,PresentMode newMode, Pixels::Format newFormat, AlphaCompositing compositing)override;

            Texture* currentImage();
            uint32_t currentImageIndex() const;
            void invalidate();
            VkSwapchainKHR vulkanHandle() const;
            friend class VulkanQueue;

        private:
            void advance();
            VkSurfaceKHR createNativeSurface(PlatformData platformData);
            void move(VulkanSwapChain& from);

            void rebuild();


            VkSurfaceKHR createVulkanWindowsSurface(Win32PlatformData data);
            VkSurfaceKHR createVulkanWaylandSurface(WaylandPlatformData data);
            VkSurfaceKHR createVulkanX11Surface(X11PlatformData data);


            FrameResources* (*createResource)(uint8_t frameIndex, SwapChain* inChain) = nullptr;
            bool _needsUpdate = true;
            VkSurfaceKHR _surface;
            uint32_t _width = 0;
            uint32_t _height = 0;
            Pixels::Format _format = Pixels::Format::UNDEFINED;
            PresentMode _presentMode = PresentMode::QUEUE;
            AlphaCompositing _compositing = AlphaCompositing::IGNORE_ALPHA;
            uint8_t _frameCount = 0;
            std::vector<VulkanFrame> _frames;
            std::vector<VulkanTexture> _images;
            std::vector<VkImageView> _imageViews;
            uint8_t _currentFrameIndex = 0;
            uint32_t _currentImageIndex = 0;
            VkSwapchainKHR _swapChain = nullptr;
            bool _frameSubmitted = true;

            FrameResources* (*_createResource)(uint8_t frameIndex, SwapChain* inChain)=nullptr;
            void (*_swapchainRebuiltFunction)(SwapChain* swapChain)=nullptr;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSWAPCHAIN_H
