#include "VulkanFrame.h"
#include "VulkanSwapchain.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VkImage backBuffer, uint32_t width, uint32_t height, uint32_t imageIndex, VkImageUsageFlags flags, VulkanSwapchain* from)
        {
            _from = from;
            _backBuffer = new VulkanTexture(backBuffer, false,_from->imageFormat(),width,height,1,flags, true);
            _commandBuffer = new VulkanCommandBuffer(VulkanLib::card()->graphicsQueueFamily());
            _imageIndex = imageIndex;
        }

        VulkanFrame::~VulkanFrame()
        {
            if(_backBuffer)
            {
                delete _backBuffer;
                delete _commandBuffer;
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

        void VulkanFrame::present()
        {
            present(nullptr,0);
        }

        void VulkanFrame::present(SemaphoreValue* signals, size_t signalCount)
        {
            uint64_t waitValue = 1;
            std::vector<VkSemaphore> signalsArray(signalCount);
            std::vector<uint64_t> signalsValues(signalCount);
            for(auto i=0; i< signalCount; i++)
            {
                signalsArray[i] = dynamic_cast<VulkanSemaphore*>(signals[i].semaphore)->semaphore();
                signalsValues[i] = signals[i].value;
            }
            VkTimelineSemaphoreSubmitInfo timelineInfo;
            timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            timelineInfo.pNext = nullptr;
            timelineInfo.waitSemaphoreValueCount = 1;
            timelineInfo.pWaitSemaphoreValues = &waitValue;
            timelineInfo.signalSemaphoreValueCount = signalCount;
            timelineInfo.pSignalSemaphoreValues = signalsValues.data();


            auto sem = _commandBuffer->_finished->semaphore();
            auto chain = _from->vulkanSwapchain();
            VkPresentInfoKHR presentInfo{};
            presentInfo.pNext = &timelineInfo;
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.pImageIndices = &_imageIndex;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &sem;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &chain;


            auto result = vkQueuePresentKHR(VulkanLib::card()->presentQueue(),&presentInfo);
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