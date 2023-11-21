#ifndef SLAG_VULKANGPUVERTEXBUFFER_H
#define SLAG_VULKANGPUVERTEXBUFFER_H
#include "../../VertexBuffer.h"
#include "VulkanGPUBuffer.h"
#include "VulkanVertexBuffer.h"

namespace slag{
    namespace vulkan
    {
        class VulkanGPUVertexBuffer: public virtual VulkanVertexBuffer, virtual VulkanGPUBuffer
        {
        public:
            VulkanGPUVertexBuffer(void* data, size_t dataLength,bool destroyImmediate);
            ~VulkanGPUVertexBuffer()=default;
            VulkanGPUVertexBuffer(VulkanGPUVertexBuffer&)=delete;
            VulkanGPUVertexBuffer& operator=(VulkanGPUVertexBuffer&)=delete;
            VulkanGPUVertexBuffer(VulkanGPUVertexBuffer&& from);
            VulkanGPUVertexBuffer& operator=(VulkanGPUVertexBuffer&& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANGPUVERTEXBUFFER_H