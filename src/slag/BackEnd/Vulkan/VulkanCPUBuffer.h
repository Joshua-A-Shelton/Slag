#ifndef SLAG_VULKANCPUBUFFER_H
#define SLAG_VULKANCPUBUFFER_H
#include "VulkanBuffer.h"
#include "../Resource.h"
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanCPUBuffer: public virtual VulkanBuffer, Resource
        {
        public:
            VulkanCPUBuffer(void* data, size_t dataLength, VkBufferUsageFlags usageFlags);
            ~VulkanCPUBuffer()override;
            VulkanCPUBuffer(const VulkanCPUBuffer&)=delete;
            VulkanCPUBuffer& operator=(const VulkanCPUBuffer&)=delete;
            VulkanCPUBuffer(VulkanCPUBuffer&& from);
            VulkanCPUBuffer& operator=(VulkanCPUBuffer&& from);
            size_t size()override;
            Usage usage()override;
            void update(size_t offset, void* data, size_t dataLength)override;
            void rebuild(size_t newSize)override;
            void* GPUID()override;
            VkBuffer underlyingBuffer()override;
        private:
            void* _mappedLocation = nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            size_t _bufferSize=0;
            VkBufferUsageFlags _flags;
            void move(VulkanCPUBuffer& from);
            void build(void* data, size_t dataLength, VkBufferUsageFlags usageFlags);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANCPUBUFFER_H