#ifndef CRUCIBLEEDITOR_VULKANGPUDATABUFFER_H
#define CRUCIBLEEDITOR_VULKANGPUDATABUFFER_H

#include "VulkanDataBuffer.h"
#include "VulkanGPUBuffer.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanGPUDataBuffer: public virtual VulkanDataBuffer, virtual VulkanGPUBuffer
        {
        public:
            VulkanGPUDataBuffer(void* data, size_t dataLength,bool destroyImmediate);
            ~VulkanGPUDataBuffer()=default;
            VulkanGPUDataBuffer(VulkanGPUDataBuffer&)=delete;
            VulkanGPUDataBuffer& operator=(VulkanGPUDataBuffer&)=delete;
            VulkanGPUDataBuffer(VulkanGPUDataBuffer&& from);
            VulkanGPUDataBuffer& operator=(VulkanGPUDataBuffer&& from);
        };

    } // vulkan
} // slag

#endif //CRUCIBLEEDITOR_VULKANGPUDATABUFFER_H
