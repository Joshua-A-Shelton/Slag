#include <iostream>
#include "VulkanSwapchain.h"
#include "VulkanFrame.h"
#include "VulkanLib.h"


namespace slag
{
    namespace vulkan
    {
        VulkanFrame::VulkanFrame(VulkanSwapchain* from, VkDeviceSize uniformBufferStartSize): _commandBuffer(nullptr)
        {
            assert(from != nullptr && "From swapchain cannot be null!");
            _fromSwapChain = from;
            auto commandPool = _fromSwapChain->commandPool();

            _commandBuffer = std::move(VulkanCommandBuffer(commandPool));

            //in flight fence, render and image available semaphores

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            auto result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_imageAvailable);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_renderFinished);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateFence(VulkanLib::graphicsCard()->device(), &fenceInfo, nullptr,&_inFlight);
            assert(result == VK_SUCCESS && "failed to create fence!");
        }

        VulkanFrame::~VulkanFrame()
        {
            if(_inFlight)
            {
                vkDestroyFence(VulkanLib::graphicsCard()->device(),_inFlight, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_imageAvailable, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_renderFinished, nullptr);
                freeResourceReferences();
            }

        }

        VulkanFrame::VulkanFrame(VulkanFrame&& from): _commandBuffer(nullptr)
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
            std::swap(_swapchainImageTexture , from._swapchainImageTexture);
            std::swap(_inFlight , from._inFlight);
            std::swap(_imageAvailable, from._imageAvailable);
            std::swap(_renderFinished, from._renderFinished);
            std::swap(_fromSwapChain, from._fromSwapChain);
            _descriptorAllocator = std::move(from._descriptorAllocator);
            _virtualUniformBuffer = std::move(from._virtualUniformBuffer);
            _commandBuffer = std::move(from._commandBuffer);

        }

        void VulkanFrame::waitTillFinished()
        {
            auto result = vkWaitForFences(VulkanLib::graphicsCard()->device(),1,&_inFlight, true, 1000000000);
            assert(result == VK_SUCCESS && "render fence wait timed out");
        }

        void VulkanFrame::resetWait()
        {
            auto result = vkResetFences(VulkanLib::graphicsCard()->device(),1,&_inFlight);
            assert(result == VK_SUCCESS && "render fence could not be reset");
            //kinda a weird spot for it, but it's technically right....
            freeResourceReferences();
        }

        VkSemaphore VulkanFrame::renderFinishedSemaphore()
        {
            return _renderFinished;
        }

        VkSemaphore VulkanFrame::imageAvailableSemaphore()
        {
            return _imageAvailable;
        }

        CommandBuffer* VulkanFrame::getCommandBuffer()
        {
            return &_commandBuffer;
        }

        Texture *VulkanFrame::getBackBuffer()
        {
            return _swapchainImageTexture;
        }

        void VulkanFrame::begin()
        {
            _descriptorAllocator.resetPools();
            _commandBuffer.reset();
            _virtualUniformBuffer.reset();
            _commandBuffer.begin();
        }

        void VulkanFrame::end()
        {
            _commandBuffer.end();

            _commandBuffer.submit(&_imageAvailable,1,&_renderFinished,1,VulkanLib::graphicsCard()->graphicsQueue(),_inFlight);
            _fromSwapChain->queueToPresent(this);
        }

        void VulkanFrame::setSwapchainImageTexture(VulkanTexture* texture)
        {
            _swapchainImageTexture = texture;
        }

        VkDeviceSize VulkanFrame::uniformBufferSize()
        {
            return _virtualUniformBuffer.virtualSize();
        }




    } // slag
} // vulkan