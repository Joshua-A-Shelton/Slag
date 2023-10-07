#ifndef SLAG_VULKANFRAME_H
#define SLAG_VULKANFRAME_H
#include "../../Frame.h"
#include "VulkanTexture.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDescriptorAllocator.h"
#include "VulkanVirtualUniformBuffer.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapchain;
        class VulkanFrame: public Frame
        {
        public:
            VulkanFrame(VulkanSwapchain* from, VkDeviceSize uniformBufferStartSize);
            VulkanFrame(const VulkanFrame&)=delete;
            VulkanFrame(VulkanFrame&& from);
            VulkanFrame& operator=(VulkanFrame&& from);
            VulkanFrame& operator=(const VulkanFrame&)=delete;
            ~VulkanFrame() override;
            void begin() override;
            void end() override;
            CommandBuffer* getCommandBuffer() override;
            Texture* getBackBuffer()override;

            VkSemaphore renderFinishedSemaphore();
            VkSemaphore imageAvailableSemaphore();
            ///only call from swapchain itself
            void setSwapchainImageTexture(VulkanTexture* texture);

            VkDeviceSize uniformBufferSize();

            void waitTillFinished();
            void resetWait();
        private:
            //is the command buffer done
            VkFence _inFlight = nullptr;
            VkSemaphore _renderFinished = nullptr;
            VkSemaphore _imageAvailable = nullptr;
            VulkanSwapchain* _fromSwapChain = nullptr;
            VulkanDescriptorAllocator _descriptorAllocator;
            VulkanVirtualUniformBuffer _virtualUniformBuffer;
            VulkanTexture* _swapchainImageTexture;
            VulkanCommandBuffer _commandBuffer;
            void move(VulkanFrame&& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANFRAME_H