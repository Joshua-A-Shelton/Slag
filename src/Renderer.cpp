#include "Renderer.h"
#include "Surface.h"

#include "BackEnd/SwapChain.h"
#include <VkBootstrap.h>

namespace slag
{
    Renderer::Renderer(Surface *surface, bool includeDepth)
    {
        _surface = surface;
        GraphicsCard* card = surface->_graphicsCard;

        _vkDevice = card->_vkDevice;
        _swapchain = new SwapChain(surface,includeDepth);

    }

    slag::Renderer::~Renderer()
    {
        SwapChain* chain = static_cast<SwapChain*>(_swapchain);
        delete chain;
    }

    void Renderer::beginFrame()
    {
        assert(!_hasStartedFrame && "Frame in progress, must call endFrame");
        SwapChain* swapChain = static_cast<SwapChain*>(_swapchain);
        auto result = swapChain->getNext();
        if(result ==VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR )
        {
            recreateSwapChain();
            swapChain = static_cast<SwapChain*>(_swapchain);
            swapChain->getNext();
        }
        _hasStartedFrame = true;
        auto commandBuffer = swapChain->getCurrentCommandBuffer();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }
        _currentSwapchainCommandBuffer = commandBuffer;

    }

    void Renderer::endFrame()
    {
        assert(_hasStartedFrame && "No frame in progress, must call beginFrame");

        if (vkEndCommandBuffer(static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer)) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);
        auto result = static_cast<SwapChain*>(_swapchain)->submitCommandBuffer(&commandBuffer);

        _hasStartedFrame = false;
    }

    void Renderer::recreateSwapChain()
    {
        vkDeviceWaitIdle(static_cast<VkDevice>(_vkDevice));
        SwapChain* currentChain = static_cast<SwapChain*>(_swapchain);
        SwapChain* newChain = new SwapChain(_surface,currentChain,currentChain->getCurrentFrameBuffer()->hasDepthBuffer());
        delete currentChain;
        _swapchain = newChain;
    }

    void Renderer::setDefaultClearColor(float * color)
    {
        auto swapchain = static_cast<SwapChain*>(_swapchain);
        for(int i=0; i< swapchain->_frameBuffers.size(); i++)
        {
            //SELECT JUST THE CLEAR COLOR
            swapchain->_frameBuffers[i]->setClearColors(color,1);
        }
    }

    void Renderer::bindDefaultFrameBuffer()
    {

        SwapChain* currentChain = static_cast<SwapChain*>(_swapchain);
        auto frameBuffer = currentChain->getCurrentFrameBuffer();
        bindFrameBuffer(frameBuffer);
    }

    void Renderer::bindFrameBuffer(std::shared_ptr<FrameBuffer> frameBuffer)
    {
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = static_cast<VkRenderPass>(frameBuffer->_vkRenderPass);
        renderPassInfo.framebuffer = static_cast<VkFramebuffer>(frameBuffer->_vkFrameBuffer);

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {frameBuffer->width(),frameBuffer->height()};

        std::vector<VkClearValue> clearColors(frameBuffer->attachmentCount());
        for(int i=0; i< frameBuffer->_clearColors.size(); i++)
        {
            clearColors[i] = VkClearValue{frameBuffer->_clearColors[i][0],frameBuffer->_clearColors[i][1],frameBuffer->_clearColors[i][2],frameBuffer->_clearColors[i][3]};
        }
        if(clearColors.size()!=frameBuffer->colorAttachmentCount())
        {
            clearColors[clearColors.size()-1] = VkClearValue {1.0f,0};
        }
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearColors.size());
        renderPassInfo.pClearValues = reinterpret_cast<const VkClearValue *>(clearColors.data());

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        //TODO: remove this and set it as its own commands
        VkViewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = static_cast<float>(frameBuffer->width());
        viewport.height = static_cast<float>(frameBuffer->height());
        viewport.minDepth = 0;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{{0,0},
                         {frameBuffer->width(),frameBuffer->height()}};
        vkCmdSetViewport(commandBuffer,0,1,&viewport);
        vkCmdSetScissor(commandBuffer,0,1,&scissor);

    }

    void Renderer::unBindCurrentFrameBuffer()
    {
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);
        vkCmdEndRenderPass(commandBuffer);
    }

    void Renderer::bindShader(Shader& shader)
    {
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);
        vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<VkPipeline>(shader._vkPipeline));
    }

    void Renderer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);
        vkCmdDraw(commandBuffer,vertexCount,instanceCount,firstVertex,firstInstance);
    }

    void Renderer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        auto commandBuffer = static_cast<VkCommandBuffer>(_currentSwapchainCommandBuffer);
        vkCmdDrawIndexed(commandBuffer,indexCount,instanceCount,firstIndex,vertexOffset,firstInstance);
    }


}