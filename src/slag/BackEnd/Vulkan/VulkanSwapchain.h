#ifndef SLAG_VULKANSWAPCHAIN_H
#define SLAG_VULKANSWAPCHAIN_H
#include "../../Swapchain.h"
#include "VulkanFrame.h"
#include "VulkanTexture.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapchain: public Swapchain
        {
        public:
            VulkanSwapchain(PlatformData platformData,
                            uint32_t width,
                            uint32_t height,
                            size_t desiredBackbuffers,
                            bool vsync,
                            std::unordered_map<std::string,TextureResourceDescription>& textureDescriptions,
                            std::unordered_set<std::string>& commandBufferNames,
                            std::unordered_map<std::string, UniformBufferResourceDescription>& uniformBufferDescriptions
                            );
            ~VulkanSwapchain() override;
            Frame* currentFrame()override;
            Frame* next()override;
            size_t backBufferCount()override;
            void backBufferCount(size_t count)override;
            bool vsyncEnabled()override;
            void vsyncEnabled(bool enabled)override;
            uint32_t width()override;
            uint32_t height()override;
            void resize(uint32_t width, uint32_t height)override;
            Pixels::PixelFormat imageFormat()override;
            VkCommandPool commandPool();
            void queueToPresent(VulkanFrame* frame);


        private:
            VkSurfaceKHR _surface = nullptr;
            VkCommandPool _commandPool = nullptr;
            VkSwapchainKHR _swapchain = nullptr;
            size_t _currentFrameIndex = 0;
            uint32_t _swapchainImageIndex = 0;
            std::vector<VulkanFrame> _frames;
            std::vector<VulkanTexture> _swapchainImages;
            VkPresentModeKHR _presentMode;
            VkFormat _defaultImageFormat;
            size_t _desiredBackbufferCount = 0;
            uint32_t _width=0;
            uint32_t _height=0;
            bool _needsRebuild= false;
            std::unordered_map<std::string,TextureResourceDescription> _textureDescriptions;
            std::unordered_set<std::string> _commandBufferNames;
            std::unordered_map<std::string, UniformBufferResourceDescription> _uniformBufferDescriptions;
            void rebuild();
            void cleanup();
            VkSurfaceKHR createNativeSurface(PlatformData platformData);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANSWAPCHAIN_H