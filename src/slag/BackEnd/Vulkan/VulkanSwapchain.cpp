#include "VulkanSwapchain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSwapchain::VulkanSwapchain(uint32_t width, uint32_t height, uint8_t backBuffers, bool vsync, Swapchain::PresentMode mode)
        {
            _width = width;
            _height = height;
            _backBufferCount = backBuffers;
            _vsync = vsync;
            _presentMode = mode;
            rebuild();
        }
    } // vulkan
} // slag