#include "VulkanFrame.h"

#include "VulkanGraphicsCard.h"
#include "VulkanSemaphore.h"
#include "VulkanSwapChain.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/core/Texture.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(uint8_t frameIndex, VulkanSwapChain* parent, FrameResources* resources):Frame(resources)
        {
            if (resources == nullptr)
            {
                throw std::invalid_argument("resources cannot be null");
            }
            _parent = parent;
            _frameIndex = frameIndex;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            vkCreateSemaphore(VulkanGraphicsCard::selected()->device(),&semaphoreInfo,nullptr,&_imageAcquiredSemaphore);
            vkCreateSemaphore(VulkanGraphicsCard::selected()->device(),&semaphoreInfo,nullptr,&_commandsCompleteSemaphore);

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            vkCreateFence(VulkanGraphicsCard::selected()->device(),&fenceInfo,nullptr,&_commandsCompleteFence);
            vkCreateFence(VulkanGraphicsCard::selected()->device(),&fenceInfo,nullptr,&_imageAcquiredFence);

        }

        VulkanFrame::~VulkanFrame()
        {
            if (_commandsCompleteFence)
            {
                vkWaitForFences(VulkanGraphicsCard::selected()->device(),1,&_commandsCompleteFence,VK_TRUE,UINT64_MAX);
                vkWaitForFences(VulkanGraphicsCard::selected()->device(),1,&_imageAcquiredFence,VK_TRUE,UINT64_MAX);
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _imageAcquiredSemaphore, nullptr);
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _commandsCompleteSemaphore, nullptr);
                vkDestroyFence(VulkanGraphicsCard::selected()->device(), _imageAcquiredFence, nullptr);
                vkDestroyFence(VulkanGraphicsCard::selected()->device(), _commandsCompleteFence, nullptr);
            }
        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from): Frame(nullptr)
        {
            move(from);
        }

        VulkanFrame& VulkanFrame::operator=(VulkanFrame&& from)
        {
            move(from);
            return *this;
        }

        Texture* VulkanFrame::backBuffer()
        {
            SLAG_ASSERT(_parent->currentFrameIndex() == _frameIndex && "Can only acquire a back buffer if we are the current frame");
            return _parent->currentImage();
        }

        uint8_t VulkanFrame::frameIndex()
        {
            return _frameIndex;
        }

        VulkanSwapChain* VulkanFrame::parentSwapChain() const
        {
            return _parent;
        }

        VkSemaphore VulkanFrame::imageAcquiredSemaphore() const
        {
            return _imageAcquiredSemaphore;
        }

        VkFence VulkanFrame::commandsCompleteFence() const
        {
            return _commandsCompleteFence;
        }

        VkFence VulkanFrame::imageAquiredFence() const
        {
            return _commandsCompleteFence;
        }

        VkSemaphore VulkanFrame::commandsCompleteSemaphore() const
        {
            return _commandsCompleteSemaphore;
        }

        void VulkanFrame::move(VulkanFrame& from)
        {
            Frame::move(from);
            std::swap(_parent, from._parent);
            _frameIndex = from._frameIndex;
            std::swap(_imageAcquiredSemaphore, from._imageAcquiredSemaphore);
            std::swap(_imageAcquiredFence, from._imageAcquiredFence);
            std::swap(_commandsCompleteSemaphore, from._commandsCompleteSemaphore);
            std::swap(_commandsCompleteFence, from._commandsCompleteFence);
        }
    } // vulkan
} // slag
