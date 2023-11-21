#include "VulkanCPUIndexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCPUIndexBuffer::VulkanCPUIndexBuffer(void* data, size_t dataLength, bool destroyImmediate): VulkanCPUBuffer(data,dataLength,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,destroyImmediate)
        {
        }

        VulkanCPUIndexBuffer::VulkanCPUIndexBuffer(VulkanCPUIndexBuffer&& from): VulkanCPUBuffer(std::move(from))
        {

        }

        VulkanCPUIndexBuffer& VulkanCPUIndexBuffer::operator=(VulkanCPUIndexBuffer&& from)
        {
            VulkanCPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // slag
} // vulkan