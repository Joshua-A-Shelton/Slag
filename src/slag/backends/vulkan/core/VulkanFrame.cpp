#include "VulkanFrame.h"

#include <iostream>

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
            auto device = VulkanGraphicsCard::selected()->device();
            _parent = parent;
            _frameIndex = frameIndex;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;


            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            vkCreateFence(device,&fenceInfo,nullptr,&_imageAcquiredFence);
            vkCreateFence(device,&fenceInfo,nullptr,&_frameFinsishedFence);

            _backBufferToGeneral = new VulkanCommandBuffer(GPUQueue::QueueType::GRAPHICS);
            vkCreateSemaphore(device,&semaphoreInfo,nullptr,&_backBufferToGeneralSemaphore);

            vkCreateSemaphore(device,&semaphoreInfo,nullptr,&_mainCommandsCompleteSemaphore);

            _backBufferToPresent = new VulkanCommandBuffer(GPUQueue::QueueType::GRAPHICS);

        }

        VulkanFrame::~VulkanFrame()
        {
            if (_frameFinsishedFence)
            {
                auto device = VulkanGraphicsCard::selected()->device();

                vkWaitForFences(device,1,&_frameFinsishedFence,true,1000000000);

                vkDestroyFence(device, _imageAcquiredFence, nullptr);
                vkDestroyFence(device, _frameFinsishedFence, nullptr);

                delete _backBufferToGeneral;
                vkDestroySemaphore(device, _backBufferToGeneralSemaphore, nullptr);
                vkDestroySemaphore(device, _mainCommandsCompleteSemaphore, nullptr);
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

        VkFence VulkanFrame::imageAcquiredFence() const
        {
            return _imageAcquiredFence;
        }

        VkFence VulkanFrame::frameFinishedFence() const
        {
            return _frameFinsishedFence;
        }

        VkSemaphore VulkanFrame::backBufferToGeneralSemaphore() const
        {
            return _backBufferToGeneralSemaphore;
        }

        VkSemaphore VulkanFrame::mainCommandsCompleteSemaphore() const
        {
            return _mainCommandsCompleteSemaphore;
        }

        VulkanCommandBuffer* VulkanFrame::backBufferToGeneral() const
        {
            return _backBufferToGeneral;
        }

        VulkanCommandBuffer* VulkanFrame::backBufferToPresent() const
        {
            return _backBufferToPresent;
        }

        void VulkanFrame::move(VulkanFrame& from)
        {
            Frame::move(from);
            std::swap(_parent, from._parent);
            _frameIndex = from._frameIndex;

            std::swap(_imageAcquiredFence,from._imageAcquiredFence);
            std::swap( _frameFinsishedFence,from._frameFinsishedFence);

            std::swap( _backBufferToGeneral ,from._backBufferToGeneral);
            std::swap( _backBufferToGeneralSemaphore ,from._backBufferToGeneralSemaphore);

            std::swap( _mainCommandsCompleteSemaphore,from._mainCommandsCompleteSemaphore);

            std::swap(_backBufferToPresent,from._backBufferToPresent);
        }
    } // vulkan
} // slag
