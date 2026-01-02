#ifndef SLAG_VULKANFRAME_H
#define SLAG_VULKANFRAME_H
#include <slag/Slag.h>

#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanTexture.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanSwapChain;
        class VulkanFrame: public Frame
        {
        public:
            VulkanFrame(uint8_t frameIndex, VulkanSwapChain* parent, FrameResources* resources);
            ~VulkanFrame()override;
            VulkanFrame(const VulkanFrame&) = delete;
            VulkanFrame& operator=(const VulkanFrame&) = delete;
            VulkanFrame(VulkanFrame&& from);
            VulkanFrame& operator=(VulkanFrame&& from);
            virtual Texture* backBuffer() override;
            virtual uint8_t frameIndex() override;
            VulkanSwapChain* parentSwapChain() const;
            VkFence imageAcquiredFence() const;
            VkFence frameFinishedFence() const;
            VulkanCommandBuffer* backBufferToGeneral() const;
            VkSemaphore backBufferToGeneralSemaphore() const;
            VkSemaphore mainCommandsCompleteSemaphore() const;
            VulkanCommandBuffer* backBufferToPresent() const;

        protected:
            void move(VulkanFrame& from);
            VulkanSwapChain* _parent = nullptr;
            uint32_t _frameIndex = 0;
            //TODO: there may be reason to make this a semaphore, and wait on the GPU until the image is acquired, rather than wait on the CPU until it's acquired, but there's been a lot of syncronization issues between platforms, so this is it for now
            VkFence _imageAcquiredFence = nullptr;
            VkFence _frameFinsishedFence = nullptr;

            VulkanCommandBuffer* _backBufferToGeneral = nullptr;
            VkSemaphore _backBufferToGeneralSemaphore = nullptr;

            VkSemaphore _mainCommandsCompleteSemaphore = nullptr;

            VulkanCommandBuffer* _backBufferToPresent = nullptr;


        };
    } // vulkan
} // slag

#endif //SLAG_VULKANFRAME_H
