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
            VkSemaphore imageAcquiredSemaphore() const;
            VkSemaphore commandsCompleteSemaphore() const;
            VkFence commandsCompleteFence() const;
            VkFence imageAcquiredFence() const;

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            VkSemaphore submittedCompleteSemaphore() const;
            VkSemaphore backBufferToGeneralSemaphore() const;
            VulkanCommandBuffer* backBufferToGeneral();
            VulkanCommandBuffer* backBufferToPresent();
#endif

        protected:
            void move(VulkanFrame& from);
            VulkanSwapChain* _parent = nullptr;
            uint32_t _frameIndex = 0;

            VkSemaphore _imageAcquiredSemaphore = nullptr;
            VkFence _imageAcquiredFence = nullptr;

            VkSemaphore _commandsCompleteSemaphore = nullptr;
            VkFence _commandsCompleteFence = nullptr;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            VkSemaphore _submittedCompleteSemaphore = nullptr;
            VulkanCommandBuffer* _backBufferToGeneral = nullptr;
            VkSemaphore _backBufferToGeneralSemaphore = nullptr;
            VulkanCommandBuffer* _backBufferToPresent = nullptr;
#endif


        };
    } // vulkan
} // slag

#endif //SLAG_VULKANFRAME_H
