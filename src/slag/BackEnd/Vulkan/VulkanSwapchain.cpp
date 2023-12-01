#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux
#define VK_USE_PLATFORM_XLIB_KHR
#endif
#include "VulkanSwapchain.h"
#include "VulkanLib.h"
#include <algorithm>
namespace slag
{
    namespace vulkan
    {
        VulkanSwapchain::VulkanSwapchain(PlatformData platformData,
                                         uint32_t width,
                                         uint32_t height,
                                         size_t desiredBackbuffers,
                                         Pixels::PixelFormat desiredFormat,
                                         bool vsync,
                                         std::unordered_map<std::string,TextureResourceDescription>& textureDescriptions,
                                         std::unordered_set<std::string>& commandBufferNames,
                                         std::unordered_map<std::string, UniformBufferResourceDescription>& uniformBufferDescriptions,
                                         std::unordered_map<std::string, VertexBufferResourceDescription>& vertexBufferDescriptions,
                                         std::unordered_map<std::string, IndexBufferResourceDescription>& indexBufferDescriptions
                                        )
        {
            _surface = createNativeSurface(platformData);
            _width = width;
            _height = height;
            _desiredBackbufferCount = std::clamp(static_cast<int>(desiredBackbuffers),1,3);
            _defaultImageFormat = VulkanTexture::formatFromCrossPlatform(desiredFormat);
            if(vsync && _desiredBackbufferCount > 1)
            {
                if(_desiredBackbufferCount == 2)
                {
                    _presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else
                {
                    _presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                }

            }
            else
            {
                _presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
            _textureDescriptions = textureDescriptions;
            _commandBufferNames = commandBufferNames;
            _uniformBufferDescriptions = uniformBufferDescriptions;
            _vertexBufferDescriptions = vertexBufferDescriptions;
            _indexBufferDescriptions = indexBufferDescriptions;

            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            //the command pool will be one that can submit graphics commands
            commandPoolInfo.queueFamilyIndex = VulkanLib::graphicsCard()->graphicsQueueFamily();
            //we also want the pool to allow for resetting of individual command buffers
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(VulkanLib::graphicsCard()->device(), &commandPoolInfo, nullptr, &_commandPool)!=VK_SUCCESS)
            {
                throw std::runtime_error("Unable to initialize Graphics Card Command Pool");
            }

            rebuild();

        }

        VulkanSwapchain::~VulkanSwapchain()
        {
            vkDeviceWaitIdle(VulkanLib::graphicsCard()->device());
            //anything that gets deferred deleted must be deleted before the frames are destroyed
            _swapchainImages.clear();
            _frames.clear();
            vkDestroySwapchainKHR(VulkanLib::graphicsCard()->device(),_swapchain, nullptr);
            _swapchain = nullptr;
            vkDestroyCommandPool(VulkanLib::graphicsCard()->device(),_commandPool, nullptr);
            _commandPool = nullptr;
            vkDestroySurfaceKHR(VulkanLib::instance(),_surface, nullptr);
            _surface = nullptr;
        }

        Frame* VulkanSwapchain::currentFrame()
        {
            if(_frames.empty())
            {
                return nullptr;
            }
            return &_frames[_currentFrameIndex];
        }

        Frame* VulkanSwapchain::next()
        {
            if(_width == 0 || _height == 0)
            {
                return nullptr;
            }

            _frames[_currentFrameIndex].waitTillFinished();


            VkResult result = vkAcquireNextImageKHR(VulkanLib::graphicsCard()->device(), _swapchain, 1000000000, _frames[_currentFrameIndex].imageAvailableSemaphore(), nullptr, &_swapchainImageIndex);
            if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needsRebuild)
            {
                rebuild();
                vkAcquireNextImageKHR(VulkanLib::graphicsCard()->device(), _swapchain, 1000000000, _frames[_currentFrameIndex].imageAvailableSemaphore(), nullptr, &_swapchainImageIndex);
            }
            else if(result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to acquire next image in swap chain");
            }
            _frames[_currentFrameIndex].resetWait();
            _frames[_currentFrameIndex].setSwapchainImageTexture(&_swapchainImages[_swapchainImageIndex]);
            return &_frames[_currentFrameIndex];
        }

        size_t VulkanSwapchain::backBufferCount()
        {
            return _frames.size();
        }

        void VulkanSwapchain::backBufferCount(size_t count)
        {
            if(count > 3)
            {
                count = 3;
            }
            else if(count == 0)
            {
                count = 1;
            }
            _desiredBackbufferCount = count;
            rebuild();
        }

        bool VulkanSwapchain::vsyncEnabled()
        {
            if(_presentMode == VK_PRESENT_MODE_FIFO_KHR || _presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return true;
            }
            return false;
        }

        void VulkanSwapchain::vsyncEnabled(bool enabled)
        {
            if(enabled && _desiredBackbufferCount > 1)
            {
                if(_desiredBackbufferCount == 2)
                {
                    _presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
                else
                {
                    _presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                }
            }
            rebuild();
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
            rebuild();
        }

        Pixels::PixelFormat VulkanSwapchain::imageFormat()
        {
            return VulkanTexture::formatFromNative(_defaultImageFormat);
        }

        void VulkanSwapchain::rebuild()
        {
            vkDeviceWaitIdle(VulkanLib::graphicsCard()->device());
            VulkanGraphicsCard* card = VulkanLib::graphicsCard();
            vkb::SwapchainBuilder swapchainBuilder{card->physicalDevice(),card->device(),_surface};

            auto vkbSwapchain = swapchainBuilder
                    .set_desired_format(VkSurfaceFormatKHR{_defaultImageFormat, VK_COLORSPACE_SRGB_NONLINEAR_KHR})
                    .set_desired_present_mode(_presentMode)
                    .set_desired_extent(_width,_height)
                    .set_desired_min_image_count(_desiredBackbufferCount)
                    .set_old_swapchain(_swapchain)
                    .build().value();
            if(_swapchain)
            {
                cleanup();
            }
            else if(_frames.size()!=0)
            {
                //LOG_ERROR("Unable to rebuild swapchain");
            }
            _presentMode = vkbSwapchain.present_mode;
            _swapchain = vkbSwapchain.swapchain;
            _width = vkbSwapchain.extent.width;
            _height = vkbSwapchain.extent.height;
            _defaultImageFormat = vkbSwapchain.image_format;
            auto images = vkbSwapchain.get_images().value();
            auto views = vkbSwapchain.get_image_views().value();
            _swapchainImages.clear();

            for(auto i=0; i< images.size(); i++)
            {
                _swapchainImages.emplace_back(std::move(VulkanTexture(images[i],views[i],_defaultImageFormat,VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT,_width,_height,true)));
            }

            if(_frames.size()>_swapchainImages.size())
            {
                while(_swapchainImages.size()>_frames.size())
                {
                    _frames.pop_back();
                }
            }
            else
            {
                VkDeviceSize bufferSize = 1250000;
                if(_frames.size()>0)
                {
                    bufferSize = _frames[0].uniformBufferSize();
                }
                while(_frames.size()<_swapchainImages.size())
                {
                    _frames.emplace_back(this,bufferSize,_textureDescriptions,_commandBufferNames,_uniformBufferDescriptions,_vertexBufferDescriptions,_indexBufferDescriptions);
                }
            }
            vkResetCommandPool(VulkanLib::graphicsCard()->device(),_commandPool,0);
            _needsRebuild = false;

        }

        void VulkanSwapchain::cleanup()
        {
            vkDeviceWaitIdle(VulkanLib::graphicsCard()->device());

            //destroying the swapchain automatically destroys the images
            vkDestroySwapchainKHR(VulkanLib::graphicsCard()->device(),_swapchain, nullptr);
            _swapchain = nullptr;
            _currentFrameIndex = 0;
        }

        VkCommandPool VulkanSwapchain::commandPool()
        {
            return _commandPool;
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
            vkCreateWin32SurfaceKHR(VulkanLib::instance(),&createInfo, nullptr,&surface);
#elif __linux
            VkXlibSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
            createInfo.window = reinterpret_cast<Window>(platformData.nativeWindowHandle);
            createInfo.dpy = static_cast<Display*>(platformData.nativeDisplayType);
            vkCreateXlibSurfaceKHR(VulkanLib::instance(),&createInfo, nullptr,&surface);
#endif
            assert(surface!= nullptr && "Unable to make rendering surface");
            return surface;
        }

        void VulkanSwapchain::queueToPresent(VulkanFrame* frame)
        {
            VkSemaphore renderFinished = frame->renderFinishedSemaphore();
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderFinished;

            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &_swapchain;

            presentInfo.pImageIndices = &_swapchainImageIndex;

            auto& card = VulkanLib::graphicsCard;

            auto result = vkQueuePresentKHR(card()->graphicsQueue(), &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _needsRebuild)
            {
                _needsRebuild = true;
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }
            //increment frame index
            _currentFrameIndex = (_currentFrameIndex + 1) % _frames.size();
        }


    } // slag
} // vulkan