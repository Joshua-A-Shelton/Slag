#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include "VulkanSwapchain.h"
#include "VkBootstrap.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSwapchain::VulkanSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode,  Pixels::Format imageFormat)
        {
            _surface = createNativeSurface(platformData);
            _width = width;
            _height = height;
            _backBufferCount = backBuffers;
            _presentMode = mode;
            _imageFormat = imageFormat;

            rebuild();
        }

        VulkanSwapchain::~VulkanSwapchain()
        {
            if(_swapchain)
            {
                vkDeviceWaitIdle(VulkanLib::card()->device());
                for (int i = 0; i < _frames.size(); i++)
                {
                    _frames[i].commandBuffer()->waitUntilFinished();
                }
                _frames.clear();
                vkDestroySwapchainKHR(VulkanLib::card()->device(), _swapchain, nullptr);
                vkDestroySurfaceKHR(VulkanLib::get()->instance(), _surface, nullptr);
                for(auto i=0; i<_imageAcquiredSemaphores.size(); i++)
                {
                    vkDestroySemaphore(VulkanLib::card()->device(),_imageAcquiredSemaphores[i], nullptr);
                    vkDestroyFence(VulkanLib::card()->device(),_imageAcquiredFences[i], nullptr);
                }
            }
        }

        VulkanSwapchain::VulkanSwapchain(VulkanSwapchain&& from)
        {
            move(std::move(from));
        }

        VulkanSwapchain& VulkanSwapchain::operator=(VulkanSwapchain&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanSwapchain::move(VulkanSwapchain&& from)
        {
            std::swap(_surface,from._surface);
            std::swap(_imageFormat,from._imageFormat);
            _width = from._width;
            _height = from._height;
            _backBufferCount = from._backBufferCount;
            _presentMode = from._presentMode;

            _currentFrameIndex = from._currentFrameIndex;
            _currentSemaphoreIndex = from._currentSemaphoreIndex;
            _needsRebuild = from._needsRebuild;
            _frames.swap(from._frames);
            _imageAcquiredSemaphores.swap(from._imageAcquiredSemaphores);
            _imageAcquiredFences.swap(from._imageAcquiredFences);
            for(auto i=0; i<_frames.size(); i++)
            {
                _frames[i]._from = this;
            }
        }

        VkSurfaceKHR VulkanSwapchain::createNativeSurface(PlatformData platformData)
        {
            VkSurfaceKHR surface = nullptr;
#ifdef _WIN32
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = static_cast<HWND>(platformData.nativeWindowHandle);
            //kinda messy, but apparently correct?
            createInfo.hinstance = static_cast<HINSTANCE>(platformData.nativeDisplayType);
            vkCreateWin32SurfaceKHR(VulkanLib::get()->instance(),&createInfo, nullptr,&surface);
#elif __linux
            //TODO: include wayland
            VkXlibSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createInfo.window = reinterpret_cast<Window>(platformData.nativeWindowHandle);
            createInfo.dpy = static_cast<Display*>(platformData.nativeDisplayType);
            vkCreateXlibSurfaceKHR(VulkanLib::get()->instance(),&createInfo, nullptr,&surface);
#endif
            assert(surface!= nullptr && "Unable to make rendering surface");
            return surface;
        }

        void VulkanSwapchain::rebuild()
        {
            vkDeviceWaitIdle(VulkanLib::card()->device());
            for(int i=0; i< _frames.size(); i++)
            {
                _frames[i].commandBuffer()->waitUntilFinished();
            }
            if(_imageAcquiredFences.size()>0)
            {
                vkWaitForFences(VulkanLib::card()->device(), _imageAcquiredFences.size(), _imageAcquiredFences.data(), true, 1000000);
            }
            VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
            if(_presentMode == PresentMode::FIFO)
            {
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
            }
            else if(_presentMode == PresentMode::MAILBOX)
            {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            }

            auto localFormat = VulkanLib::format(_imageFormat);
            vkb::SwapchainBuilder swapchainBuilder(VulkanLib::card()->physicalDevice(),VulkanLib::card()->device(),_surface);
            auto chain = swapchainBuilder.set_desired_format(VkSurfaceFormatKHR{localFormat.format,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                    .set_desired_present_mode(presentMode)
                    .set_desired_extent(_width,_height)
                    .set_desired_min_image_count(_backBufferCount)
                    //.set_desired_min_image_count()
                    .set_old_swapchain(_swapchain)
                    .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                    .build();
            if(_swapchain)
            {
                vkDestroySwapchainKHR(VulkanLib::card()->device(),_swapchain, nullptr);
                _frames.clear();
                for(auto i=0; i<_imageAcquiredSemaphores.size(); i++)
                {
                    vkDestroySemaphore(VulkanLib::card()->device(),_imageAcquiredSemaphores[i], nullptr);
                    vkDestroyFence(VulkanLib::card()->device(),_imageAcquiredFences[i], nullptr);
                }
                _imageAcquiredSemaphores.clear();
                _imageAcquiredFences.clear();
            }
            _swapchain = chain.value();
            if(chain->present_mode == VK_PRESENT_MODE_FIFO_KHR)
            {
                _presentMode = PresentMode::FIFO;
            }
            else if(chain->present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                _presentMode = PresentMode::MAILBOX;
            }
            auto images = chain->get_images().value();
            for(int i=0; i< images.size(); i++)
            {
                VulkanFrame frame(images[i],_width,_height,i,chain->image_usage_flags,this);
                _frames.push_back(std::move(frame));

                VkSemaphoreCreateInfo semaphoreCreateInfo{};
                semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                VkSemaphore semaphore = nullptr;
                auto value = vkCreateSemaphore(VulkanLib::card()->device(),&semaphoreCreateInfo, nullptr,&semaphore);
                _imageAcquiredSemaphores.push_back(semaphore);

                VkFenceCreateInfo fenceCreateInfo{};
                fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                VkFence fence = nullptr;
                vkCreateFence(VulkanLib::card()->device(),&fenceCreateInfo, nullptr,&fence);
                _imageAcquiredFences.push_back(fence);

            }

            _currentFrameIndex = 0;
            _currentSemaphoreIndex = 0;
            _needsRebuild = false;
        }

        Frame* VulkanSwapchain::next()
        {
            if(_width == 0 || _height == 0)
            {
                return nullptr;
            }

            //_frames[_currentFrameIndex].commandBuffer()->waitUntilFinished();
            auto fence = currentImageAcquiredFence();
            vkWaitForFences(VulkanLib::card()->device(),1,&fence,true,1000000000);
            vkResetFences(VulkanLib::card()->device(),1,&fence);

            auto result = vkAcquireNextImageKHR(VulkanLib::card()->device(), _swapchain, 1000000000, _imageAcquiredSemaphores[_currentSemaphoreIndex], nullptr, &_currentFrameIndex);


            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needsRebuild)
            {
                rebuild();
                auto fence = currentImageAcquiredFence();
                vkWaitForFences(VulkanLib::card()->device(),1,&fence,true,1000000000);
                vkResetFences(VulkanLib::card()->device(),1,&fence);
                vkAcquireNextImageKHR(VulkanLib::card()->device(), _swapchain, 1000000000, _imageAcquiredSemaphores[_currentSemaphoreIndex], nullptr, &_currentFrameIndex);
            }
            else if(result != VK_SUCCESS)
            {
                throw std::runtime_error("unable to acquire next frame");
            }

            return &_frames[_currentFrameIndex];
        }

        Frame* VulkanSwapchain::nextIfReady()
        {
            if(_width == 0 || _height == 0)
            {
                return nullptr;
            }

            auto fence = currentImageAcquiredFence();
            if(vkGetFenceStatus(VulkanLib::card()->device(),fence) == VK_SUCCESS)
            {
                vkResetFences(VulkanLib::card()->device(),1,&fence);

                auto result = vkAcquireNextImageKHR(VulkanLib::card()->device(), _swapchain, 1000000000, _imageAcquiredSemaphores[_currentSemaphoreIndex], nullptr, &_currentFrameIndex);


                if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needsRebuild)
                {
                    rebuild();
                    auto fence = currentImageAcquiredFence();
                    vkWaitForFences(VulkanLib::card()->device(),1,&fence,true,1000000000);
                    vkResetFences(VulkanLib::card()->device(),1,&fence);
                    vkAcquireNextImageKHR(VulkanLib::card()->device(), _swapchain, 1000000000, _imageAcquiredSemaphores[_currentSemaphoreIndex], nullptr, &_currentFrameIndex);
                }
                else if(result != VK_SUCCESS)
                {
                    throw std::runtime_error("unable to acquire next frame");
                }

                return &_frames[_currentFrameIndex];
            }
            return nullptr;
        }

        Frame* VulkanSwapchain::currentFrame()
        {
            return &_frames[_currentFrameIndex];
        }

        uint8_t VulkanSwapchain::currentFrameIndex()
        {
            return _currentFrameIndex;
        }

        uint8_t VulkanSwapchain::backBuffers()
        {
            return _frames.size();
        }

        void VulkanSwapchain::backBuffers(uint8_t count)
        {
            _backBufferCount = count;
            _needsRebuild = true;
        }

        uint32_t VulkanSwapchain::width()
        {
            return _width;
        }

        uint32_t VulkanSwapchain::height()
        {
            return _height;
        }

        void VulkanSwapchain::resize(uint32_t width, uint32_t height)
        {
            _width = width;
            _height = height;
            _needsRebuild = true;
        }

        Swapchain::PresentMode VulkanSwapchain::presentMode()
        {
            return _presentMode;
        }

        void VulkanSwapchain::presentMode(Swapchain::PresentMode mode)
        {
            _presentMode = mode;
            _needsRebuild = true;
        }

        Pixels::Format VulkanSwapchain::imageFormat()
        {
            return _imageFormat;
        }

        bool VulkanSwapchain::needsRebuild()
        {
            return _needsRebuild;
        }

        VkSwapchainKHR VulkanSwapchain::vulkanSwapchain()
        {
            return _swapchain;
        }

        VkSemaphore VulkanSwapchain::currentImageAcquiredSemaphore()
        {
            return _imageAcquiredSemaphores[_currentSemaphoreIndex];
        }

        VkFence VulkanSwapchain::currentImageAcquiredFence()
        {
            return _imageAcquiredFences[_currentSemaphoreIndex];
        }

        void VulkanSwapchain::finishedFrame()
        {
            _currentSemaphoreIndex = (_currentSemaphoreIndex + 1) % _imageAcquiredSemaphores.size();
        }

    } // vulkan
} // slag