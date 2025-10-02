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
            vkCreateSemaphore(VulkanGraphicsCard::selected()->device(),&semaphoreInfo,nullptr,&_submittedCompleteSemaphore);
            _backBufferToGeneral = new VulkanCommandBuffer(GPUQueue::QueueType::GRAPHICS);
            vkCreateSemaphore(VulkanGraphicsCard::selected()->device(),&semaphoreInfo,nullptr,&_backBufferToGeneralSemaphore);
            _backBufferToPresent = new VulkanCommandBuffer(GPUQueue::QueueType::GRAPHICS);

        }

        VulkanFrame::~VulkanFrame()
        {
            if (_commandsCompleteFence)
            {
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _imageAcquiredSemaphore, nullptr);
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _commandsCompleteSemaphore, nullptr);
                vkDestroyFence(VulkanGraphicsCard::selected()->device(), _commandsCompleteFence, nullptr);
                vkDestroyFence(VulkanGraphicsCard::selected()->device(), _imageAcquiredFence, nullptr);
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _submittedCompleteSemaphore, nullptr);
                delete _backBufferToGeneral;
                vkDestroySemaphore(VulkanGraphicsCard::selected()->device(), _backBufferToGeneralSemaphore, nullptr);
                delete _backBufferToPresent;
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

        VkFence VulkanFrame::imageAcquiredFence() const
        {
            return _imageAcquiredFence;
        }

        VkSemaphore VulkanFrame::submittedCompleteSemaphore() const
        {
            return _submittedCompleteSemaphore;
        }

        VkSemaphore VulkanFrame::backBufferToGeneralSemaphore() const
        {
            return _backBufferToGeneralSemaphore;
        }

        VulkanCommandBuffer* VulkanFrame::backBufferToGeneral()
        {
            return _backBufferToGeneral;
        }

        VulkanCommandBuffer* VulkanFrame::backBufferToPresent()
        {
            return _backBufferToPresent;
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
            std::swap(_submittedCompleteSemaphore, from._submittedCompleteSemaphore);
            std::swap(_backBufferToGeneral,from._backBufferToGeneral);
            std::swap(_backBufferToGeneralSemaphore,from._backBufferToGeneralSemaphore);
            std::swap(_backBufferToPresent,from._backBufferToPresent);
        }
    } // vulkan
} // slag
