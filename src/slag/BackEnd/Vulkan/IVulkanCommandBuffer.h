#ifndef SLAG_IVULKANCOMMANDBUFFER_H
#define SLAG_IVULKANCOMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "slag/Rectangle.h"
#include "VulkanBuffer.h"
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


            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)override;
            void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)override;
            void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)override;

            //vulkan only operations
            void vulkanBlitImage(VkImageAspectFlags aspects, VulkanTexture* source, VkImageLayout sourceLayout, Rectangle sourceArea, uint32_t sourceLayer, uint32_t sourceMipLevel, VulkanTexture* destination, VkImageLayout destImageLayout, Rectangle destArea, uint32_t destLayer, uint32_t destMipLevel, VkFilter filter);
            VkCommandBuffer underlyingCommandBuffer();

        protected:
            void move(IVulkanCommandBuffer& from);
            VkCommandBuffer _buffer = nullptr;
            VkCommandPool _pool = nullptr;
            uint32_t _family = 0;
        };

    } // vulkan
} // slag

#endif //SLAG_IVULKANCOMMANDBUFFER_H
