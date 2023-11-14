#ifndef SLAG_VULKANBUFFER_H
#define SLAG_VULKANBUFFER_H
#include "../../Buffer.h"
#include "../Resource.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanBuffer: public Buffer, Resource
        {
        public:
            VulkanBuffer(void* data, size_t bufferSize, Type type, Usage usage);
            VulkanBuffer(const VulkanBuffer&)=delete;
            VulkanBuffer& operator=(const VulkanBuffer&)=delete;
            VulkanBuffer(VulkanBuffer&& from);
            VulkanBuffer& operator=(VulkanBuffer&& from);
            Usage usage()override;
            virtual size_t size()override;
            Type type()override;
            void update(size_t offset,void* data, size_t dataSize)override;
            void* GPUID()override;
            VkBuffer& underlyingBuffer();
            ~VulkanBuffer()override;
        private:
            void* _mappedLocation = nullptr;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation = nullptr;
            size_t _size;
            Usage _usage;
            Type _type;
            void move(VulkanBuffer&& from);
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANBUFFER_H