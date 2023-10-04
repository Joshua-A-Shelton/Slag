#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer cmdBuffer)
        {
            _cmdBuffer = cmdBuffer;
        }

        VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&from)
        {
            move(std::move(from));
        }

        VulkanCommandBuffer &VulkanCommandBuffer::operator=(VulkanCommandBuffer &&from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanCommandBuffer::move(VulkanCommandBuffer &&from)
        {
            std::swap(_cmdBuffer, from._cmdBuffer);
        }

    } // slag
} // vulkan