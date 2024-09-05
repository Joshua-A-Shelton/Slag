#include "VulkanFrame.h"
#include "VulkanSwapchain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VkImage backBuffer, uint32_t width, uint32_t height, uint32_t imageIndex, VkImageUsageFlags flags, VulkanSwapchain* from)
        {
            _from = from;
            _backBuffer = new VulkanTexture(backBuffer, false,_from->imageFormat(),width,height,1,flags, VK_IMAGE_ASPECT_COLOR_BIT, true);
            _commandBuffer = new VulkanCommandBuffer(VulkanLib::card()->graphicsQueueFamily());
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
                delete _commandBuffer;
                vkDestroySemaphore(VulkanLib::card()->device(),_commandsFinished, nullptr);
            }
        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from)
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
            std::swap(_backBuffer,from._backBuffer);
            std::swap(_commandBuffer,from._commandBuffer);
            std::swap(_from,from._from);
            std::swap(_commandsFinished,from._commandsFinished);
            _imageIndex = from._imageIndex;
        }

        Texture* VulkanFrame::backBuffer()
        {
            return _backBuffer;
        }

        CommandBuffer* VulkanFrame::commandBuffer()
        {
            return _commandBuffer;
        }

        void VulkanFrame::begin()
        {
            _commandBuffer->begin();
        }

        void VulkanFrame::end()
        {
            _commandBuffer->end();
            dynamic_cast<VulkanQueue*>(VulkanLib::card()->graphicsQueue())->submit(_commandBuffer,_from->currentImageAcquiredSemaphore(),_commandsFinished);
            auto swapchain = _from->vulkanSwapchain();
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pImageIndices = &_imageIndex;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &_commandsFinished;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.swapchainCount = 1;

            auto result = vkQueuePresentKHR(dynamic_cast<VulkanQueue*>(VulkanLib::card()->graphicsQueue())->underlyingQueue(),&presentInfo);
            _from->finishedFrame();
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _from->needsRebuild())
            {
                _from->rebuild();
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }
        }

    } // vulkan
} // slag