#include "VulkanGPUVertexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGPUVertexBuffer::VulkanGPUVertexBuffer(void* data, size_t dataLength,bool destroyImmediate): VulkanGPUBuffer(data,dataLength,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, destroyImmediate)
        {
        }

        VulkanGPUVertexBuffer::VulkanGPUVertexBuffer(VulkanGPUVertexBuffer&& from): VulkanGPUBuffer(std::move(from))
        {

        }

        VulkanGPUVertexBuffer& VulkanGPUVertexBuffer::operator=(VulkanGPUVertexBuffer&& from)
        {
            VulkanGPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // slag
} // vulkan