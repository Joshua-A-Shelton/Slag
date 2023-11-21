#ifndef SLAG_VULKANCPUVERTEXBUFFER_H
#define SLAG_VULKANCPUVERTEXBUFFER_H

#include "VulkanVertexBuffer.h"
#include "VulkanCPUBuffer.h"

namespace slag{
    namespace vulkan
    {
        class VulkanCPUVertexBuffer: public virtual VulkanVertexBuffer, virtual VulkanCPUBuffer
        {
        public:
            VulkanCPUVertexBuffer(void* data, size_t dataLength, bool destroyImmediate);
            ~VulkanCPUVertexBuffer()=default;
            VulkanCPUVertexBuffer(VulkanCPUVertexBuffer&)=delete;
            VulkanCPUVertexBuffer& operator=(VulkanCPUVertexBuffer&)=delete;
            VulkanCPUVertexBuffer(VulkanCPUVertexBuffer&& from);
            VulkanCPUVertexBuffer& operator=(VulkanCPUVertexBuffer&& from);
        };
    } // slag
} // Vulkan
#endif //SLAG_VULKANCPUVERTEXBUFFER_H