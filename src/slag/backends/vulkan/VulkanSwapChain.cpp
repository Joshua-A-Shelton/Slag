#ifdef SLAG_WIN32_BACKEND
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifdef SLAG_X11_BACKEND
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#ifdef SLAG_WAYLAND_BACKEND
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include "VulkanSwapChain.h"

#include "VulkanBackend.h"
#include "VulkanGraphicsCard.h"
#include <slag/exceptions/ResourceCreationError.h>

#include "VulkanSubmissionQueue.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSwapChain::VulkanSwapChain(
            VulkanGraphicsCard* graphicsCard,
            const PlatformData& platformData,
            uint32_t width,
            uint32_t height,
            const SwapChainParameters& parameters)
        {
            _surface = createNativeSurface(platformData);
            _parameters = parameters;
            _graphicsCard = graphicsCard;
            _width = width;
            _height = height;
            rebuild();
        }

        VulkanSwapChain::VulkanSwapChain(VulkanSwapChain&& from) noexcept
        {
            move(from);
        }

        VulkanSwapChain& VulkanSwapChain::operator=(VulkanSwapChain&& from) noexcept
        {
            move(from);
            return *this;
        }

        VulkanSwapChain::~VulkanSwapChain()
        {
            if (_swapChain)
            {
                _frames.clear();
                vkQueueWaitIdle(static_cast<VulkanSubmissionQueue*>(_graphicsCard->graphicsQueue())->vulkanHandle());
                vkDestroySwapchainKHR(_graphicsCard->device(), _swapChain, nullptr);
                vkDestroySurfaceKHR(static_cast<VulkanBackend*>(Slag::backend())->instance().instance, _surface, nullptr);
                for (auto& fence : _imageAcquiredFence)
                {
                    vkDestroyFence(_graphicsCard->device(), fence, nullptr);
                }
            }
        }

        Frame* VulkanSwapChain::next()
        {
            auto result = vkAcquireNextImageKHR(_graphicsCard->device(), _swapChain, UINT64_MAX, nullptr, _imageAcquiredFence[_currentFenceIndex], &_currentFrameIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                rebuild();
                result = vkAcquireNextImageKHR(_graphicsCard->device(), _swapChain, UINT64_MAX, nullptr, _imageAcquiredFence[_currentFenceIndex], &_currentFrameIndex);
            }
            if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw ResourceCreationError("Unable to acquire next image");
            }
            vkWaitForFences(_graphicsCard->device(), 1, &_imageAcquiredFence[_currentFenceIndex], VK_TRUE, UINT64_MAX);
            vkResetFences(_graphicsCard->device(), 1, &_imageAcquiredFence[_currentFenceIndex]);
            _currentFenceIndex = (_currentFenceIndex + 1) % _imageAcquiredFence.size();
            return &_frames[_currentFrameIndex];
        }

        Frame* VulkanSwapChain::currentFrame()
        {
            return &_frames[_currentFrameIndex];
        }

        void VulkanSwapChain::present()
        {
            VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            presentInfo.waitSemaphoreCount = 0;
            presentInfo.pWaitSemaphores = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &_swapChain;
            presentInfo.pImageIndices = &_currentFrameIndex;

            VkResult result = vkQueuePresentKHR(static_cast<VulkanSubmissionQueue*>(_graphicsCard->graphicsQueue())->vulkanHandle(), &presentInfo);
        }

        const SwapChainParameters& VulkanSwapChain::parameters() const
        {
            return _parameters;
        }

        GraphicsCard* VulkanSwapChain::graphicsCard()
        {
            return _graphicsCard;
        }

        void VulkanSwapChain::rebuild()
        {
            _frames.clear();
            for (int i=0;i<_imageAcquiredFence.size();i++)
            {
                vkDestroyFence(_graphicsCard->device(),_imageAcquiredFence[i],nullptr);
            }
            _imageAcquiredFence.clear();

            vkb::SwapchainBuilder swapchainBuilder(_graphicsCard->physicalDevice(), _graphicsCard->device(), _surface);
            auto format = VulkanBackend::nativeFormat(_parameters.imageFormat);
            auto chain = swapchainBuilder
                .set_desired_format(VkSurfaceFormatKHR{format.format,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                .set_desired_extent(_width, _height)
                .set_desired_min_image_count(_parameters.imageCount)
                .set_old_swapchain(_swapChain)
                .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
                .set_composite_alpha_flags(VulkanBackend::nativeCompositeAlphaFlags(_parameters.alphaCompositing))
                .set_desired_present_mode(VulkanBackend::nativePresentMode(_parameters.presentMode))
                .build();
            if (!chain.has_value())
            {
                throw ResourceCreationError("Unable to rebuild swap chain");
            }

            auto images = chain->get_images().value();
            auto views = chain->get_image_views().value();

            auto crossFormat = VulkanBackend::crossPlatformFormat(chain->image_format);
            for (auto i=0u;i<images.size();i++)
            {
                _frames.emplace_back(i,images[i],views[i],chain->extent.width,chain->extent.height,crossFormat,this);

                VkFenceCreateInfo fenceCreateInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
                VkFence fence{};
                vkCreateFence(_graphicsCard->device(),&fenceCreateInfo,nullptr,&fence);
                _imageAcquiredFence.push_back(fence);
            }
            _width = chain->extent.width;
            _height = chain->extent.height;
            if (_swapChain !=nullptr)
            {
                //TODO: there's probably a better way to do this
                vkQueueWaitIdle(static_cast<VulkanSubmissionQueue*>(_graphicsCard->graphicsQueue())->vulkanHandle());
                vkDestroySwapchainKHR(_graphicsCard->device(),_swapChain,nullptr);
            }
            _swapChain = chain->swapchain;
            _parameters.imageCount = images.size();
            _parameters.presentMode = VulkanBackend::crossPlatformPresentMode(chain->present_mode);
            _parameters.imageFormat = crossFormat;

            _currentFrameIndex = 0;
            _currentFenceIndex = 0;
        }

        void VulkanSwapChain::move(VulkanSwapChain& from) noexcept
        {
            _surface = from._surface;
            _parameters = from._parameters;
            _graphicsCard = from._graphicsCard;
            _width = from._width;
            _height = from._height;
            _swapChain = from._swapChain;
            _frames = std::move(from._frames);
            _imageAcquiredFence = std::move(from._imageAcquiredFence);
            _currentFrameIndex = from._currentFrameIndex;
            _currentFenceIndex = from._currentFenceIndex;
        }

        VkSurfaceKHR VulkanSwapChain::createNativeSurface(const PlatformData& platformData)
        {
            switch (platformData.platform)
            {
            case Platform::WIN_32:
                return createVulkanWindowsSurface(platformData.details.win32);
                break;
            case Platform::WAYLAND:
                return createVulkanWaylandSurface(platformData.details.wayland);
                break;
            case Platform::X11:
                return createVulkanX11Surface(platformData.details.x11);
                break;
            default:
                throw std::runtime_error("Unsupported platform");
            }
        }

         VkSurfaceKHR VulkanSwapChain::createVulkanWindowsSurface(Win32PlatformData data)
        {
#ifdef SLAG_WIN32_BACKEND
            VkSurfaceKHR surface = nullptr;
            VkWin32SurfaceCreateInfoKHR createWindowsInfo{};
            createWindowsInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createWindowsInfo.hwnd = static_cast<HWND>(data.hwnd);
            //kinda messy, but apparently correct?
            createWindowsInfo.hinstance = static_cast<HINSTANCE>(data.hinstance);
            vkCreateWin32SurfaceKHR(((VulkanBackend*)Slag::backend())->instance(),&createWindowsInfo, nullptr,&surface);
            return surface;
#else
            throw std::runtime_error("Cannot initialize Vulkan Swapchain on Windows backend");
#endif
        }

        VkSurfaceKHR VulkanSwapChain::createVulkanWaylandSurface(WaylandPlatformData data)
        {
#ifdef SLAG_WAYLAND_BACKEND
            VkSurfaceKHR surface = nullptr;

            VkWaylandSurfaceCreateInfoKHR createWaylandInfo{};
            createWaylandInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            createWaylandInfo.display = static_cast<wl_display*>(data.display);
            createWaylandInfo.surface = static_cast<wl_surface*>(data.surface);
            vkCreateWaylandSurfaceKHR(((VulkanBackend*)Slag::backend())->instance(),&createWaylandInfo, nullptr,&surface);
            return surface;
#else
            throw std::runtime_error("Cannot initialize Vulkan Swapchain on X11 backend");
#endif
        }

        VkSurfaceKHR VulkanSwapChain::createVulkanX11Surface(X11PlatformData data)
        {
#if SLAG_X11_BACKEND
            VkSurfaceKHR surface = nullptr;

            VkXlibSurfaceCreateInfoKHR createX11Info{};
            createX11Info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createX11Info.window = reinterpret_cast<Window>(data.window);
            createX11Info.dpy = static_cast<Display*>(data.display);
            vkCreateXlibSurfaceKHR(((VulkanBackend*)Slag::backend())->instance(),&createX11Info, nullptr,&surface);
            return surface;
#else
            throw std::runtime_error("Cannot initialize Vulkan Swapchain on X11 backend");
#endif
        }
    } // vulkan
} // slag