#include "IVulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanGraphicsCard.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
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
            throw NotImplemented();
        }

        void IVulkanCommandBuffer::copyBufferToTexture(
            Buffer* source,
            Texture* destination,
            TextureBufferMapping* copyData,
            uint32_t mappingCount)
        {
            throw NotImplemented();
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
