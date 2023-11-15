#include "VulkanCPUVertexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCPUVertexBuffer::VulkanCPUVertexBuffer(void* data, size_t dataLength): VulkanCPUBuffer(data,dataLength,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
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