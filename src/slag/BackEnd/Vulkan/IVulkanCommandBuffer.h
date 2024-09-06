#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "slag/Rectangle.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;
        class IVulkanCommandBuffer: public CommandBuffer
        {
        public:
            virtual ~IVulkanCommandBuffer()=default;
            void end()override;

            GpuQueue::QueueType commandType()override;


            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)override;
            VkCommandBuffer underlyingCommandBuffer();
            //vulkan only operations
            void transitionImageSubResource(VulkanTexture* texture, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t startingMipLevel, uint32_t levelCount, uint32_t startingLayer = 0, uint32_t layerCount =1);
            void blitSubResource(VkImageAspectFlags aspects, VulkanTexture* source, VkImageLayout sourceLayout, Rectangle sourceArea, uint32_t sourceMipLevel, VulkanTexture* destination, VkImageLayout destImageLayout, Rectangle destArea, uint32_t  destMipLevel, VkFilter filter);
        protected:
            VkCommandBuffer _buffer = nullptr;
            VkCommandPool _pool = nullptr;
            uint32_t _family = 0;
        };

    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
