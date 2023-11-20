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
            void rebuild(size_t newSize)override;
            void* GPUID()override;
            VkBuffer underlyingBuffer()override;
        private:
            VkBuffer _buffer= nullptr;
            VmaAllocation _allocation = nullptr;
            size_t _bufferSize=0;
            VkBufferUsageFlags _flags;
            void move(VulkanGPUBuffer& from);
            void build(void* data, size_t dataLength, VkBufferUsageFlags usageFlags);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANGPUBUFFER_H