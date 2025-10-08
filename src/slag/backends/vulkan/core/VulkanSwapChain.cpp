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
#include "VkBootstrap.h"
#include "VulkanGraphicsCard.h"
#include "slag/backends/Backend.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSwapChain::VulkanSwapChain(PlatformData platformData, uint32_t width, uint32_t height,PresentMode presentMode, uint8_t frameCount, Pixels::Format format, AlphaCompositing compositing, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain), void (*swapchainRebuiltFunction)(SwapChain* swapChain))
        {
            _surface = createNativeSurface(platformData);
            _width = width;
            _height = height;
            _format = format;
            _presentMode = presentMode;
            _frameCount = frameCount;
            _compositing = compositing;
            _createResource = createResourceFunction;
            _swapchainRebuiltFunction = swapchainRebuiltFunction;

            rebuild();
        }

        VulkanSwapChain::~VulkanSwapChain()
        {
            if (_swapChain)
            {
                //I hate this, but waiting on fences is behaving differently on different platforms
                vkDeviceWaitIdle(VulkanGraphicsCard::selected()->device());

                _frames.clear();
                vkDestroySwapchainKHR(VulkanGraphicsCard::selected()->device(), _swapChain, nullptr);

                auto vulkanBackend = static_cast<VulkanBackend*>(Backend::current());
                vkDestroySurfaceKHR(vulkanBackend->vulkanInstance().instance, _surface, nullptr);
                for(auto i=0; i<_imageViews.size(); i++)
                {
                    vkDestroyImageView(VulkanGraphicsCard::selected()->device(),_imageViews[i], nullptr);
                }
            }
        }

        Frame* VulkanSwapChain::next()
        {
            SLAG_ASSERT(_frameSubmitted && "current frame must be submitted (GPUQueue::submit(Frame* frame,...)) before calling next again");
            if(_width == 0 || _height == 0)
            {
                return nullptr;
            }
            auto device = VulkanGraphicsCard::selected()->device();
            auto& frame = _frames[_currentFrameIndex];

            auto commandsFinished = frame.commandsCompleteFence();
            auto imageAcquired = frame.imageAcquiredFence();

            vkWaitForFences(device,1,&imageAcquired,VK_TRUE,UINT64_MAX);
            vkResetFences(device,1,&commandsFinished);
            vkResetFences(device,1,&imageAcquired);

            auto result = vkAcquireNextImageKHR(device,_swapChain,UINT64_MAX,frame.imageAcquiredSemaphore(),nullptr,&_currentImageIndex);

            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needsUpdate)
            {
                _needsUpdate = true;
                rebuild();

                auto& rebuiltFrame = _frames[_currentFrameIndex];

                commandsFinished = rebuiltFrame.commandsCompleteFence();
                imageAcquired = frame.imageAcquiredFence();

                vkWaitForFences(device,1,&imageAcquired,VK_TRUE,UINT64_MAX);
                vkResetFences(device,1,&commandsFinished);
                vkResetFences(device,1,&imageAcquired);

                result = vkAcquireNextImageKHR(device,_swapChain,UINT64_MAX,rebuiltFrame.imageAcquiredSemaphore(),nullptr,&_currentImageIndex);
                if (result!=VK_SUCCESS)
                {
                    throw std::runtime_error("failed to acquire swap chain image");
                }
            }
            else if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to acquire swap chain image");
            }
            _frameSubmitted = false;
            return currentFrame();
        }

        Frame* VulkanSwapChain::nextIfReady()
        {
            if(_width == 0 || _height == 0)
            {
                return nullptr;
            }
            auto& frame = _frames[_currentFrameIndex];
            if (vkGetFenceStatus(VulkanGraphicsCard::selected()->device(),frame.imageAcquiredFence()) == VK_SUCCESS)
            {
                return next();
            }
            return nullptr;
        }

        Frame* VulkanSwapChain::currentFrame()
        {
            return &_frames[_currentFrameIndex];
        }

        uint8_t VulkanSwapChain::currentFrameIndex()
        {
            return _currentFrameIndex;
        }

        Pixels::Format VulkanSwapChain::backBufferFormat()
        {
            return _format;
        }

        void VulkanSwapChain::backBufferFormat(Pixels::Format newFormat)
        {
            _format = newFormat;
            _needsUpdate = true;
            if (_frameSubmitted)
            {
                rebuild();
            }
        }

        uint32_t VulkanSwapChain::backBufferWidth()
        {
            return _width;
        }

        uint32_t VulkanSwapChain::backBufferHeight()
        {
            return _height;
        }

        void VulkanSwapChain::backBufferSize(uint32_t newWidth, uint32_t newHeight)
        {
            _width = newWidth;
            _height = newHeight;
            _needsUpdate = true;
            if (_frameSubmitted)
            {
                rebuild();
            }
        }

        SwapChain::PresentMode VulkanSwapChain::presentMode()
        {
            return _presentMode;
        }

        uint8_t VulkanSwapChain::frameCount()
        {
            return _frameCount;
        }

        void VulkanSwapChain::presentMode(PresentMode newMode, uint8_t frameCount)
        {
            _presentMode = newMode;
            _frameCount = frameCount;
            _needsUpdate = true;
            if (_frameSubmitted)
            {
                rebuild();
            }
        }

        SwapChain::AlphaCompositing VulkanSwapChain::alphaCompositing()
        {
            return _compositing;
        }

        void VulkanSwapChain::alphaComposition(AlphaCompositing newCompositing)
        {
            _compositing = newCompositing;
            _needsUpdate = true;
            if (_frameSubmitted)
            {
                rebuild();
            }
        }

        void VulkanSwapChain::setProperties(uint32_t newWidth, uint32_t newHeight, uint8_t frameCount, PresentMode newMode, Pixels::Format newFormat, AlphaCompositing compositing)
        {
            _width = newWidth;
            _height = newHeight;
            _presentMode = newMode;
            _frameCount = frameCount;
            _presentMode = newMode;
            _format = newFormat;
            _compositing = compositing;
            _needsUpdate = true;
            if (_frameSubmitted)
            {
                rebuild();
            }
        }

        Texture* VulkanSwapChain::currentImage()
        {
            return &_images[_currentImageIndex];
        }

        uint32_t VulkanSwapChain::currentImageIndex() const
        {
            return _currentImageIndex;
        }

        void VulkanSwapChain::invalidate()
        {
            _needsUpdate = true;
        }

        VkSwapchainKHR VulkanSwapChain::vulkanHandle() const
        {
            return _swapChain;
        }

        void VulkanSwapChain::advance()
        {
            _currentFrameIndex = (_currentFrameIndex + 1) % _frameCount;
            _frameSubmitted = true;
            if (_needsUpdate)
            {
                rebuild();
            }
        }

        VkSurfaceKHR VulkanSwapChain::createNativeSurface(PlatformData platformData)
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

        void VulkanSwapChain::move(VulkanSwapChain& from)
        {
            _needsUpdate = from._needsUpdate;
            std::swap(_surface, from._surface);
            _width = from._width;
            _height = from._height;
            _format = from._format;
            _presentMode = from._presentMode;
            _frameCount = from._frameCount;
            _frames.swap(from._frames);
            _images.swap(from._images);
            _imageViews.swap(from._imageViews);
            _currentFrameIndex = from._currentFrameIndex;
            _currentImageIndex = from._currentImageIndex;
            std::swap(_swapChain, from._swapChain);

            _createResource = from._createResource;
        }

        void VulkanSwapChain::rebuild()
        {
            SLAG_ASSERT(_frameSubmitted && "Cannot rebuild swapchain between next and submit");
            //I hate this, but waiting on fences is behaving differently on different platforms
            vkDeviceWaitIdle(VulkanGraphicsCard::selected()->device());

            _frames.clear();
            _images.clear();
            for (auto i = 0; i < _imageViews.size(); ++i)
            {
                vkDestroyImageView(VulkanGraphicsCard::selected()->device(), _imageViews[i], nullptr);
            }
            _imageViews.clear();

            _currentFrameIndex = 0;
            _currentImageIndex = 0;

            auto presentMode = VulkanBackend::vulkanizedPresentMode(_presentMode);
            auto format = VulkanBackend::vulkanizedFormat(_format);
            auto imageUsageFlags = VulkanBackend::vulkanizedUsage(Texture::UsageFlags::RENDER_TARGET_ATTACHMENT);
            auto usageFlags = VulkanBackend::vulkanizedAspectFlags(Pixels::AspectFlags::COLOR);
            vkb::SwapchainBuilder swapchainBuilder(VulkanGraphicsCard::selected()->physicalDevice(),VulkanGraphicsCard::selected()->device(),_surface);
            auto chain = swapchainBuilder.set_desired_format(VkSurfaceFormatKHR{format.format,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                .set_desired_present_mode(presentMode)
                .set_desired_extent(_width,_height)
                .set_desired_min_image_count(_frames.size())
                .set_old_swapchain(_swapChain)
                .add_image_usage_flags(imageUsageFlags)
                .set_composite_alpha_flags(VulkanBackend::vulkanizedCompositeAlphaFlags(_compositing))
                .build()
                ;
            if (!chain.has_value())
            {
                throw std::runtime_error("Failed to build swapchain");
            }
            if (_swapChain)
            {
                vkDestroySwapchainKHR(VulkanGraphicsCard::selected()->device(),_swapChain,nullptr);
            }
            _swapChain = chain.value();
            auto images = chain->get_images().value();
            for (auto i=0; i<images.size(); i++)
            {
                VkImageView view;
                VkImageViewCreateInfo viewCreateInfo = {};
                viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewCreateInfo.flags = 0;
                viewCreateInfo.image = images[i];
                viewCreateInfo.format = format.format;
                viewCreateInfo.components = format.mapping;
                viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewCreateInfo.subresourceRange.layerCount = 1;
                viewCreateInfo.subresourceRange.baseMipLevel = 0;
                viewCreateInfo.subresourceRange.levelCount = 1;
                viewCreateInfo.subresourceRange.baseArrayLayer = 0;
                viewCreateInfo.subresourceRange.aspectMask = usageFlags;
                vkCreateImageView(VulkanGraphicsCard::selected()->device(),&viewCreateInfo,nullptr,&view);
                _images.push_back(VulkanTexture(images[i],view,_format,Texture::Type::TEXTURE_2D,_width,_height,1,1,1,Texture::UsageFlags::RENDER_TARGET_ATTACHMENT,Texture::SampleCount::ONE));
                _imageViews.push_back(view);
            }

            for (auto i=0; i<_frameCount; i++)
            {
                _frames.emplace_back(i,this,_createResource(i,this));
            }

            _needsUpdate = false;

            if (_swapchainRebuiltFunction!=nullptr)
            {
                _swapchainRebuiltFunction(this);
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
            vkCreateWin32SurfaceKHR(((VulkanBackend*)Backend::current())->vulkanInstance(),&createWindowsInfo, nullptr,&surface);
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
            vkCreateWaylandSurfaceKHR(((VulkanBackend*)Backend::current())->vulkanInstance(),&createWaylandInfo, nullptr,&surface);
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
            vkCreateXlibSurfaceKHR(((VulkanBackend*)Backend::current())->vulkanInstance(),&createX11Info, nullptr,&surface);
            return surface;
#else
            throw std::runtime_error("Cannot initialize Vulkan Swapchain on X11 backend");
#endif
        }
    } // vulkan
} // slag
