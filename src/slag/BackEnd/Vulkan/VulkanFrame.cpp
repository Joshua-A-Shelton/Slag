#include "VulkanFrame.h"
#include "VulkanSwapchain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VkImage backBuffer, uint32_t width, uint32_t height, uint32_t imageIndex, VkImageUsageFlags flags, VulkanSwapchain* from, FrameResources* frameResources): Frame(frameResources)
        {
            _from = from;
            _backBuffer = new VulkanTexture(backBuffer, false,_from->imageFormat(),width,height,1,flags, VK_IMAGE_ASPECT_COLOR_BIT, true);
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            vkCreateSemaphore(VulkanLib::card()->device(),&semaphoreCreateInfo, nullptr,&_commandsFinished);
            _imageIndex = imageIndex;

        }

        VulkanFrame::~VulkanFrame()
        {
            if(_backBuffer)
            {
                delete _backBuffer;
                vkDestroySemaphore(VulkanLib::card()->device(),_commandsFinished, nullptr);
            }
        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from): Frame(nullptr)
        {
            move(std::move(from));
        }

        VulkanFrame& VulkanFrame::operator=(VulkanFrame&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanFrame::move(VulkanFrame&& from)
        {
            Frame::move(from);
            std::swap(_backBuffer,from._backBuffer);
            std::swap(_from,from._from);
            std::swap(_commandsFinished,from._commandsFinished);
            _imageIndex = from._imageIndex;
        }

        Texture* VulkanFrame::backBuffer()
        {
            return _backBuffer;
        }

        VulkanSwapchain* VulkanFrame::from()
        {
            return _from;
        }

        VkSemaphore VulkanFrame::commandsFinishedSemaphore()
        {
            return _commandsFinished;
        }

    } // vulkan
} // slag