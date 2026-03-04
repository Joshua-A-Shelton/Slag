#include "IVulkanCommandBuffer.h"

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanTexture.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;

        GraphicsCard* IVulkanCommandBuffer::graphicsCard()
        {
            return _graphicsCard;
        }

        QueueType IVulkanCommandBuffer::type() const
        {
            return _type;
        }

        CommandBufferLevel IVulkanCommandBuffer::level() const
        {
            return _level;
        }

        void IVulkanCommandBuffer::begin()
        {
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            //TODO: I need to have this set to match DX12 functionality
            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(_commandBuffer,&cmdBeginInfo);
        }

        void IVulkanCommandBuffer::end()
        {
            vkEndCommandBuffer(_commandBuffer);
        }

        void IVulkanCommandBuffer::copyBufferToBuffer(
            Buffer* source,
            uint64_t sourceOffset,
            Buffer* destination,
            uint64_t destinationOffset,
            uint64_t length)
        {
            SLAG_ASSERT(source != nullptr && "Source buffer cannot be null");
            SLAG_ASSERT(destination != nullptr && "Destination buffer cannot be null");
            SLAG_ASSERT(sourceOffset+length <= source->size() && "Copy would exceed length of source buffer");
            SLAG_ASSERT(destinationOffset+length <= destination->size() && "Copy would exceed destination buffer");

            VulkanBuffer* src = static_cast<VulkanBuffer*>(source);
            VulkanBuffer* dst = static_cast<VulkanBuffer*>(destination);

            VkBufferCopy copyRegion = {};
            copyRegion.size = length;
            copyRegion.srcOffset = sourceOffset;
            copyRegion.dstOffset = destinationOffset;

            vkCmdCopyBuffer(_commandBuffer, src->vulkanHandle(), dst->vulkanHandle(), 1, &copyRegion);
        }

        void IVulkanCommandBuffer::copyTextureToBuffer(
            Texture* source,
            Buffer* destination,
            TextureBufferMapping* copyData,
            uint32_t mappingCount)
        {
            SLAG_ASSERT(source!=nullptr && "source cannot be null");
            SLAG_ASSERT(destination!=nullptr && "destination cannot be null");
            SLAG_ASSERT(copyData!=nullptr && "copyData cannot be null");
            SLAG_ASSERT(mappingCount>0 && "mappingCount count cannot be 0");

            auto vulkanTexture = static_cast<VulkanTexture*>(source);
            auto vulkanBuffer = static_cast<VulkanBuffer*>(destination);

            std::vector<VkBufferImageCopy> regions(mappingCount);
            for (uint32_t i = 0; i < mappingCount; ++i)
            {
                auto& region = regions[i];
                auto& subResource = copyData[i];
                auto aspectMask = VulkanBackend::nativeTextureAspect(static_cast<PixelAspectFlags>(subResource.subresource.aspect));
                region.bufferOffset = subResource.bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = aspectMask;
                region.imageSubresource.mipLevel = subResource.subresource.mipLevel;
                region.imageSubresource.baseArrayLayer = subResource.subresource.baseArrayLayer;
                region.imageSubresource.layerCount = subResource.subresource.layerCount;
                region.imageOffset = {.x = subResource.offset.x,.y = subResource.offset.y,.z = subResource.offset.z};
                region.imageExtent = {.width = subResource.extent.width,.height = subResource.extent.height,.depth = subResource.extent.depth};
            }

            vkCmdCopyImageToBuffer(_commandBuffer,vulkanTexture->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,vulkanBuffer->vulkanHandle(),mappingCount,regions.data());
        }

        void IVulkanCommandBuffer::copyBufferToTexture(
            Buffer* source,
            Texture* destination,
            TextureBufferMapping* copyData,
            uint32_t mappingCount)
        {
            SLAG_ASSERT(source!=nullptr && "buffer cannot be null");
            SLAG_ASSERT(destination!=nullptr && "texture cannot be null");
            SLAG_ASSERT(copyData!=nullptr && "copyData cannot be null");
            SLAG_ASSERT(mappingCount>0 && "mappingCount count cannot be 0");


            auto image = static_cast<VulkanTexture*>(destination);
            auto buffer = static_cast<VulkanBuffer*>(source);

            std::vector<VkBufferImageCopy> regions(mappingCount);

            for (uint32_t i = 0; i < mappingCount; ++i)
            {
                auto& region = regions[i];
                auto& subResource = copyData[i];
                auto aspectMask = VulkanBackend::nativeTextureAspect(static_cast<PixelAspectFlags>(subResource.subresource.aspect));
                region.bufferOffset = subResource.bufferOffset;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageSubresource.aspectMask = aspectMask;
                region.imageSubresource.mipLevel = subResource.subresource.mipLevel;
                region.imageSubresource.baseArrayLayer = subResource.subresource.baseArrayLayer;
                region.imageSubresource.layerCount = subResource.subresource.layerCount;
                region.imageOffset = {.x = subResource.offset.x,.y = subResource.offset.y,.z = subResource.offset.z};
                region.imageExtent = {.width = subResource.extent.width,.height = subResource.extent.height,.depth = subResource.extent.depth};
            }

            vkCmdCopyBufferToImage(_commandBuffer,buffer->vulkanHandle(),image->vulkanHandle(),VK_IMAGE_LAYOUT_GENERAL,mappingCount,regions.data());
        }

        VkCommandBuffer IVulkanCommandBuffer::vulkanHandle() const
        {
            return _commandBuffer;
        }

        void IVulkanCommandBuffer::IVKCBMove(IVulkanCommandBuffer& from)
        {
            std::swap(_commandBuffer, from._commandBuffer);
            std::swap(_commandPool, from._commandPool);
            _graphicsCard = from._graphicsCard;
            _type = from._type;
            _level = from._level;
        }
    } // vulkan
} // slag
