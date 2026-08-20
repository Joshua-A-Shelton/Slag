#ifndef SLAG_VULKANSWAPCHAIN_H
#define SLAG_VULKANSWAPCHAIN_H
#include "slag/Slag.h"
#include <vulkan/vulkan.h>

#include "VulkanFrame.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard;

        class VulkanSwapChain:public SwapChain
        {
        public:
            VulkanSwapChain(VulkanGraphicsCard* graphicsCard, const PlatformData& platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters);
            VulkanSwapChain(const VulkanSwapChain&)=delete;
            VulkanSwapChain& operator=(const VulkanSwapChain&)=delete;
            VulkanSwapChain(VulkanSwapChain&& from) noexcept;
            VulkanSwapChain& operator=(VulkanSwapChain&& from) noexcept;
            ~VulkanSwapChain()override;

            Frame* next()override;
            [[nodiscard]] Frame* currentFrame()override;
            void present()override;
            [[nodiscard]] const SwapChainParameters& parameters()const override;
            void setParameters(const SwapChainParameters& newParameters)override;
            GraphicsCard* graphicsCard()override;
        private:
            void rebuild();
            void move(VulkanSwapChain& from) noexcept;
            VkSurfaceKHR createNativeSurface(const PlatformData& platformData);
            VkSurfaceKHR createVulkanWindowsSurface(Win32PlatformData data);
            VkSurfaceKHR createVulkanWaylandSurface(WaylandPlatformData data);
            VkSurfaceKHR createVulkanX11Surface(X11PlatformData data);

            SwapChainParameters _parameters;
            std::vector<VulkanFrame> _frames;
            std::vector<VkFence> _imageAcquiredFence;
            VulkanGraphicsCard* _graphicsCard;
            VkSwapchainKHR _swapChain = nullptr;
            VkSurfaceKHR _surface = nullptr;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _currentFrameIndex = 0;
            uint32_t _currentFenceIndex = 0;
            bool _parametersChanged = false;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSWAPCHAIN_H