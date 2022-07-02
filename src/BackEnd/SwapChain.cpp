#include <iostream>
#include "SwapChain.h"
#include <limits>
namespace slag
{


    SwapChain::SwapChain(Surface* surface, bool includeDepth)
    {
        init(surface, nullptr,includeDepth);
    }

    SwapChain::SwapChain(Surface *surface, SwapChain *previous, bool includeDepth)
    {
        init(surface, previous,includeDepth);
    }

    SwapChain::~SwapChain()
    {
        GraphicsCard* card = _surface->getUnderlyingGraphicsCard();
        VkDevice logicalDevice = static_cast<VkDevice>(card->_vkDevice);


        vkFreeCommandBuffers(logicalDevice,_vkCommandPool,static_cast<uint32_t>(_vkCommandBuffers.size()),_vkCommandBuffers.data());
        _vkCommandBuffers.clear();
        vkDestroyCommandPool(logicalDevice,_vkCommandPool, nullptr);
        _frameBuffers.clear();

        vkDestroySwapchainKHR(logicalDevice,_swapchain, nullptr);
    }


    void SwapChain::init(Surface *surface, SwapChain *previous, bool includeDepth)
    {
        _surface = surface;
        GraphicsCard* card = surface->getUnderlyingGraphicsCard();
        VkPhysicalDevice physicalDevice = static_cast<VkPhysicalDevice>(card->_vkPhysicalDevice);
        VkDevice logicalDevice = static_cast<VkDevice>(card->_vkDevice);
        vkb::SwapchainBuilder swapchainBuilder{physicalDevice,logicalDevice,static_cast<VkSurfaceKHR>(surface->_vkSurface)};

        VkPresentModeKHR presentMode;
        switch (surface->currentRenderingMode())
        {
            case Surface::RenderingMode::singleBuffer:
                presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            case Surface::RenderingMode::doubleBuffer:
                presentMode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            case Surface::RenderingMode::tripleBuffer:
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
        }

        vkb::Swapchain vkbSwapchain = swapchainBuilder
                .use_default_format_selection()
                .set_desired_present_mode(presentMode)
                .set_old_swapchain(previous? previous->_swapchain: nullptr)
                .set_composite_alpha_flags(surface->_supportAlpha? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR: VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                .build()
                .value();

        _swapchain = vkbSwapchain.swapchain;
        _swapchainImages = vkbSwapchain.get_images().value();
        _swapchainImageViews = vkbSwapchain.get_image_views().value();
        _swapchainImageFormat = vkbSwapchain.image_format;
        std::shared_ptr<RenderAttachment> depth = nullptr;


        //TODO: swapchain images may not be the same resolution as the swapchain extent, but I'm currently auto setting it to. that needs to change
        if(includeDepth)
        {
            depth = RenderAttachment::CreateDepthAttachment(vkbSwapchain.extent.width,
                                                            vkbSwapchain.extent.height,
                                                            card);
        }

        for(int i=0; i< _swapchainImages.size(); i++)
        {
            std::vector<std::shared_ptr<RenderAttachment>> attachments{};
            auto color = RenderAttachment::CreateAttachmentFromResources(
                    static_cast<RenderAttachment::Format>(_swapchainImageFormat),
                    vkbSwapchain.extent.width,
                    vkbSwapchain.extent.height,
                    _swapchainImages[i],
                    _swapchainImageViews[i],
                    card,
                    RenderAttachment::AttachmentType::Color);
            attachments.push_back(color);
            if(includeDepth)
            {
                attachments.push_back(depth);
            }
            std::vector<std::array<float,4>> clearColors(1);
            clearColors[0] = {surface->clearColor[0],surface->clearColor[1],surface->clearColor[2],surface->clearColor[3]};
            _frameBuffers.push_back(FrameBuffer::Create(attachments,vkbSwapchain.extent.width,vkbSwapchain.extent.height,card,clearColors));
        }

        _vkCommandBuffers.resize(_swapchainImages.size());


        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = nullptr;
        commandPoolInfo.queueFamilyIndex = card->_graphicsQueueFamily;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VkCommandPool commandPool;
        if(vkCreateCommandPool(static_cast<VkDevice>(card->_vkDevice),&commandPoolInfo, nullptr,&commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create command pool");
        }

        _vkCommandPool = commandPool;


        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = static_cast<uint32_t>(_vkCommandBuffers.size());

        if (vkAllocateCommandBuffers(static_cast<VkDevice>(card->_vkDevice), &allocInfo,_vkCommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        _imageAvailableSemaphores.resize(_swapchainImages.size());
        _renderFinishedSemaphores.resize(_swapchainImages.size());
        _inFlightFences.resize(_swapchainImages.size());
        _imagesInFlight.resize(_swapchainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < _swapchainImages.size(); i++) {
            if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateFence(logicalDevice, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    std::shared_ptr<FrameBuffer> SwapChain::getCurrentFrameBuffer()
    {
        return _frameBuffers[_swapChainImageIndex];
    }

    VkCommandBuffer SwapChain::getCurrentCommandBuffer()
    {
        return _vkCommandBuffers[currentFrame];
    }

    VkResult SwapChain::getNext()
    {
        GraphicsCard* card = _surface->getUnderlyingGraphicsCard();
        VkDevice logicalDevice = static_cast<VkDevice>(card->_vkDevice);
        vkWaitForFences(logicalDevice,1,&_inFlightFences[currentFrame], true,UINT64_MAX);
        VkResult result = vkAcquireNextImageKHR(logicalDevice,_swapchain,UINT64_MAX,_imageAvailableSemaphores[currentFrame], nullptr, &_swapChainImageIndex);
        return result;
    }

    VkResult SwapChain::submitCommandBuffer(VkCommandBuffer* buffer)
    {
        GraphicsCard* card = _surface->getUnderlyingGraphicsCard();
        VkDevice logicalDevice = static_cast<VkDevice>(card->_vkDevice);
        if (_imagesInFlight[_swapChainImageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(logicalDevice, 1, &_imagesInFlight[_swapChainImageIndex], VK_TRUE, UINT64_MAX);
        }
        _imagesInFlight[_swapChainImageIndex] = _inFlightFences[currentFrame];

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffer;

        VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(logicalDevice, 1, &_inFlightFences[currentFrame]);
        if (vkQueueSubmit(static_cast<VkQueue>(card->_vkGraphicsQueue), 1, &submitInfo, _inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {_swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &_swapChainImageIndex;

        auto result = vkQueuePresentKHR(static_cast<VkQueue>(card->_vkPresentQueue), &presentInfo);

        currentFrame = (currentFrame + 1) % _swapchainImages.size();

        return result;
    }




}