#include <iostream>
#include "VulkanSwapchain.h"
#include "VulkanFrame.h"
#include "VulkanLib.h"


namespace slag
{
    namespace vulkan
    {
        VulkanFrame::~VulkanFrame()
        {
            if(_commandBuffer)
            {
                if(_fromSwapChain->commandPool())
                {
                    vkFreeCommandBuffers(VulkanLib::graphicsCard()->device(), _fromSwapChain->commandPool(), 1, &_commandBuffer);
                }
                vkDestroyFence(VulkanLib::graphicsCard()->device(),_inFlight, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_imageAvailable, nullptr);
                vkDestroySemaphore(VulkanLib::graphicsCard()->device(),_renderFinished, nullptr);
            }
            freeResourceReferences();
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
            std::swap(_commandBuffer,from._commandBuffer);
            std::swap(_swapchainImageTexture , from._swapchainImageTexture);
            std::swap(_inFlight , from._inFlight);
            std::swap(_imageAvailable, from._imageAvailable);
            std::swap(_renderFinished, from._renderFinished);
            std::swap(_fromSwapChain, from._fromSwapChain);
            _descriptorAllocator = std::move(from._descriptorAllocator);
            _virtualUniformBuffer = std::move(from._virtualUniformBuffer);


        }

        void VulkanFrame::waitTillFinished()
        {
            auto result = vkWaitForFences(VulkanLib::graphicsCard()->device(),1,&_inFlight, true, 1000000000);
            assert(result == VK_SUCCESS && "render fence wait timed out");
            //result = vkResetFences(VulkanLib::graphicsCard()->device(),1,&_inFlight);
            //assert(result == VK_SUCCESS && "render fence could not be reset");


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
            return nullptr;
        }

        void VulkanFrame::begin()
        {
            _descriptorAllocator.resetPools();
            vkResetCommandBuffer(_commandBuffer,0);
            _virtualUniformBuffer.reset();

            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            auto result =  vkBeginCommandBuffer(_commandBuffer, &cmdBeginInfo);
            assert(result == VK_SUCCESS && "Unable to begin command buffer");
        }

        void VulkanFrame::end()
        {
            auto result = vkEndCommandBuffer(_commandBuffer);
            assert(result==VK_SUCCESS && "Unable to end command buffer");

            VkSubmitInfo submit = {};
            submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit.pNext = nullptr;

            VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            submit.pWaitDstStageMask = &waitStage;

            submit.waitSemaphoreCount = 1;
            submit.pWaitSemaphores = &_imageAvailable;

            submit.signalSemaphoreCount = 1;
            submit.pSignalSemaphores = &_renderFinished;

            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &_commandBuffer;

            result = vkQueueSubmit(VulkanLib::graphicsCard()->graphicsQueue(), 1, &submit, _inFlight);
            assert(result == VK_SUCCESS && "Unable to submit render queue");
            _fromSwapChain->queueToPresent(this);
        }

        VulkanFrame::VulkanFrame(VulkanSwapchain* from, VkDeviceSize uniformBufferStartSize)
        {
            assert(from != nullptr && "From swapchain cannot be null!");
            _fromSwapChain = from;
            auto commandPool = _fromSwapChain->commandPool();

            //command buffer
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;

            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;
            auto result = vkAllocateCommandBuffers(VulkanLib::graphicsCard()->device(), &allocInfo, &_commandBuffer);
            assert(result == VK_SUCCESS && "failed to allocate command buffers!");

            //in flight fence, render and image available semaphores

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_imageAvailable);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateSemaphore(VulkanLib::graphicsCard()->device(),&semaphoreInfo, nullptr,&_renderFinished);
            assert(result == VK_SUCCESS && "failed to create semaphore!");
            result = vkCreateFence(VulkanLib::graphicsCard()->device(), &fenceInfo, nullptr,&_inFlight);
            assert(result == VK_SUCCESS && "failed to create fence!");
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