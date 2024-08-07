#include "VulkanGPUDataBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGPUDataBuffer::VulkanGPUDataBuffer(void* data, size_t dataLength, bool destroyImmediate): VulkanGPUBuffer(data,dataLength,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, destroyImmediate)
        {

        }

        VulkanGPUDataBuffer::VulkanGPUDataBuffer(VulkanGPUDataBuffer&& from): VulkanGPUBuffer(std::move(from))
        {

        }

        VulkanGPUDataBuffer& VulkanGPUDataBuffer::operator=(VulkanGPUDataBuffer&& from)
        {
            VulkanGPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // vulkan
} // slag