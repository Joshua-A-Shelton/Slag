#ifndef SLAG_VULKANGPUBUFFER_H
#define SLAG_VULKANGPUBUFFER_H

#include <vulkan/vulkan.h>
#include "VulkanBuffer.h"
#include "../Resource.h"
#include "vk_mem_alloc.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGPUBuffer: public virtual VulkanBuffer, Resource
        {
        public:
            VulkanGPUBuffer(void* data, size_t dataLength,VkBufferUsageFlags usageFlags);
            ~VulkanGPUBuffer()override;
            VulkanGPUBuffer(const VulkanGPUBuffer&)=delete;
            VulkanGPUBuffer& operator=(const VulkanGPUBuffer&)=delete;
            VulkanGPUBuffer(VulkanGPUBuffer&& from);
            VulkanGPUBuffer& operator=(VulkanGPUBuffer&& from);
            size_t size()override;
            Usage usage()override;
            void update(size_t offset, void* data, size_t dataLength)override;
            void* GPUID()override;
            VkBuffer underlyingBuffer()override;
        private:
            VkBuffer _buffer= nullptr;
            VmaAllocation _allocation = nullptr;
            size_t _bufferSize=0;
            void move(VulkanGPUBuffer& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANGPUBUFFER_H