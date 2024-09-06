#ifndef SLAG_VULKANCOMMANDBUFFER_H
#define SLAG_VULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "../../Resources/Resource.h"
#include "../../Resources/ResourceConsumer.h"
#include "VulkanSemaphore.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanTexture;
        class VulkanCommandBuffer: public CommandBuffer, resources::ResourceConsumer
        {
        public:
            VulkanCommandBuffer(uint32_t queueFamily);
            ~VulkanCommandBuffer()override;
            VulkanCommandBuffer(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer& operator=(const VulkanCommandBuffer&)=delete;
            VulkanCommandBuffer(VulkanCommandBuffer&& from);
            VulkanCommandBuffer& operator=(VulkanCommandBuffer&& from);

            void begin()override;
            void end()override;
            void waitUntilFinished()override;
            bool isFinished()override;
            GpuQueue::QueueType commandType()override;
            friend class VulkanQueue;
            friend class VulkanFrame;

            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)override;
            VkCommandBuffer underlyingCommandBuffer();
            //vulkan only operations
            void transitionImageSubResource(VulkanTexture* texture, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t startingMipLevel, uint32_t levelCount, uint32_t startingLayer = 0, uint32_t layerCount =1);
            void blitSubResource(VkImageAspectFlags aspects, VulkanTexture* source, VkImageLayout sourceLayout, Rectangle sourceArea, uint32_t sourceMipLevel, VulkanTexture* destination, VkImageLayout destImageLayout, Rectangle destArea, uint32_t  destMipLevel, VkFilter filter);
        private:
            void move(VulkanCommandBuffer&& from);
            void _waitUntilFinished();
            VkCommandBuffer _buffer = nullptr;
            VkCommandPool _pool = nullptr;
            VulkanSemaphore* _finished = nullptr;
            uint32_t _family = 0;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANCOMMANDBUFFER_H
