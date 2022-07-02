#ifndef SLAGLIB_SWAPCHAIN_H
#define SLAGLIB_SWAPCHAIN_H
#include <VkBootstrap.h>
#include <vector>
#include "../Surface.h"
#include "../FrameBuffer.h"
namespace slag
{
    class Renderer;
    class SwapChain
    {
    public:
        SwapChain(Surface* surface, bool includeDepth=true);
        SwapChain(Surface* surface, SwapChain* previous, bool includeDepth=true);
        SwapChain(SwapChain&)=delete;
        SwapChain& operator=(SwapChain&)=delete;
        ~SwapChain();
        std::shared_ptr<FrameBuffer> getCurrentFrameBuffer();
        VkCommandBuffer getCurrentCommandBuffer();
        VkResult getNext();
        //TODO: remove from here, put in renderer
        VkResult submitCommandBuffer(VkCommandBuffer* buffer);
    private:
        void init(Surface* surface, SwapChain* previous, bool includeDepth);
        size_t currentFrame = 0;
        VkSwapchainKHR _swapchain;
        VkFormat _swapchainImageFormat;
        std::vector<VkImage> _swapchainImages;
        std::vector<VkImageView> _swapchainImageViews;
        Surface* _surface;
        std::vector<std::shared_ptr<FrameBuffer>> _frameBuffers;
        uint32_t _swapChainImageIndex = 0;
        VkCommandPool _vkCommandPool;
        std::vector<VkCommandBuffer> _vkCommandBuffers;

        std::vector<VkFence> _inFlightFences;
        std::vector<VkFence> _imagesInFlight;
        std::vector<VkSemaphore> _imageAvailableSemaphores;
        std::vector<VkSemaphore> _renderFinishedSemaphores;
        friend class Surface;
        friend class Renderer;
        friend class CommandBuffer;
    };
}

#endif //SLAGLIB_SWAPCHAIN_H