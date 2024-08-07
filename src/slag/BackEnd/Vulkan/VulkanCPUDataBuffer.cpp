#include "VulkanCPUDataBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCPUDataBuffer::VulkanCPUDataBuffer(void* data, size_t dataLength, bool destroyImmediate): VulkanCPUBuffer(data,dataLength,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, destroyImmediate)
        {

        }

        VulkanCPUDataBuffer::VulkanCPUDataBuffer(VulkanCPUDataBuffer&& from): VulkanCPUBuffer(std::move(from))
        {

        }

        VulkanCPUDataBuffer& VulkanCPUDataBuffer::operator=(VulkanCPUDataBuffer&& from)
        {
            VulkanCPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // vulkan
} // slag