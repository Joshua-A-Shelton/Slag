#ifndef SLAG_VULKANCPUINDEXBUFFER_H
#define SLAG_VULKANCPUINDEXBUFFER_H

#include "VulkanIndexBuffer.h"
#include "VulkanCPUBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanCPUIndexBuffer: public virtual VulkanIndexBuffer, virtual VulkanCPUBuffer
        {
        public:
            VulkanCPUIndexBuffer(void* data, size_t dataLength, bool destroyImmediate);
            ~VulkanCPUIndexBuffer()=default;
            VulkanCPUIndexBuffer(VulkanCPUIndexBuffer&)=delete;
            VulkanCPUIndexBuffer& operator=(VulkanCPUIndexBuffer&)=delete;
            VulkanCPUIndexBuffer(VulkanCPUIndexBuffer&& from);
            VulkanCPUIndexBuffer& operator=(VulkanCPUIndexBuffer&& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANCPUINDEXBUFFER_H