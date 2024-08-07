#ifndef CRUCIBLEEDITOR_VULKANCPUDATABUFFER_H
#define CRUCIBLEEDITOR_VULKANCPUDATABUFFER_H
#include "VulkanDataBuffer.h"
#include "VulkanCPUBuffer.h"
namespace slag
{
    namespace vulkan
    {

        class VulkanCPUDataBuffer: public virtual VulkanDataBuffer, virtual VulkanCPUBuffer
        {
        public:
            VulkanCPUDataBuffer(void* data, size_t dataLength,bool destroyImmediate);
            ~VulkanCPUDataBuffer()=default;
            VulkanCPUDataBuffer(VulkanCPUDataBuffer&)=delete;
            VulkanCPUDataBuffer& operator=(VulkanCPUDataBuffer&)=delete;
            VulkanCPUDataBuffer(VulkanCPUDataBuffer&& from);
            VulkanCPUDataBuffer& operator=(VulkanCPUDataBuffer&& from);
        };

    } // vulkan
} // slag

#endif //CRUCIBLEEDITOR_VULKANCPUDATABUFFER_H
