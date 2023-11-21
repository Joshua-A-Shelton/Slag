#ifndef SLAG_VULKANGPUINDEXBUFFER_H
#define SLAG_VULKANGPUINDEXBUFFER_H

#include "VulkanIndexBuffer.h"
#include "VulkanGPUBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGPUIndexBuffer: public virtual VulkanIndexBuffer, virtual VulkanGPUBuffer
        {
        public:
            VulkanGPUIndexBuffer(void* data, size_t dataLength, bool destroyImmediate);
            ~VulkanGPUIndexBuffer()=default;
            VulkanGPUIndexBuffer(VulkanGPUIndexBuffer&)=delete;
            VulkanGPUIndexBuffer& operator=(VulkanGPUIndexBuffer&)=delete;
            VulkanGPUIndexBuffer(VulkanGPUIndexBuffer&& from);
            VulkanGPUIndexBuffer& operator=(VulkanGPUIndexBuffer&& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANGPUINDEXBUFFER_H