#ifndef SLAG_VULKANBUFFER_H
#define SLAG_VULKANBUFFER_H
#include "../../Buffer.h"
#include "../../Resources/Resource.h"
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanBuffer: public Buffer, resources::Resource
        {
        public:
            VulkanBuffer(void* data, size_t dataLength, Accessibility accessibility, VkBufferUsageFlags usageFlags, bool destroyImmediately);
            VulkanBuffer(size_t bufferSize, Accessibility accessibility,VkBufferUsageFlags usageFlags, bool destroyImmediately);
            ~VulkanBuffer()override;
            VulkanBuffer(const VulkanBuffer&)=delete;
            VulkanBuffer& operator=(const VulkanBuffer&)=delete;
            VulkanBuffer(VulkanBuffer&& from);
            VulkanBuffer& operator=(VulkanBuffer&& from);
            void update(size_t offset, void* data, size_t dataLength)override;
            std::vector<std::byte> downloadData()override;
            size_t size()override;
            Accessibility accessibility()override;
            unsigned char* cpuHandle()override;
            VkBuffer underlyingBuffer();
        private:
            void move(VulkanBuffer&& from);
            void updateCPU(size_t offset, void* data, size_t dataLength);
            void updateGPU(size_t offset, void* data, size_t dataLength);
            std::vector<std::byte> downloadDataCPU();
            std::vector<std::byte> downloadDataGPU();
            VkBuffer _buffer= nullptr;
            size_t _size = 0;
            VmaAllocation _allocation = nullptr;
            Accessibility _accessibility=CPU_AND_GPU;
            Usage _usage = static_cast<Usage>(0);
            void* _memoryLocation = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANBUFFER_H
