#include "VulkanCPUVertexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCPUVertexBuffer::VulkanCPUVertexBuffer(void* data, size_t dataLength, bool destroyImmediate): VulkanCPUBuffer(data,dataLength,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, destroyImmediate)
        {
        }

        VulkanCPUVertexBuffer::VulkanCPUVertexBuffer(VulkanCPUVertexBuffer&& from): VulkanCPUBuffer(std::move(from))
        {

        }

        VulkanCPUVertexBuffer& VulkanCPUVertexBuffer::operator=(VulkanCPUVertexBuffer&& from)
        {
            VulkanCPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // slag
} // Vulkan