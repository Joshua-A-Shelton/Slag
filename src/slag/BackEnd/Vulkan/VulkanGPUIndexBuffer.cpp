#include "VulkanGPUIndexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGPUIndexBuffer::VulkanGPUIndexBuffer(void* data, size_t dataLength, bool destroyImmediate): VulkanGPUBuffer(data,dataLength,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,destroyImmediate)
        {
        }

        VulkanGPUIndexBuffer::VulkanGPUIndexBuffer(VulkanGPUIndexBuffer&& from): VulkanGPUBuffer(std::move(from))
        {

        }

        VulkanGPUIndexBuffer& VulkanGPUIndexBuffer::operator=(VulkanGPUIndexBuffer&& from)
        {
            VulkanGPUBuffer::operator=(std::move(from));
            return *this;
        }
    } // slag
} // vulkan