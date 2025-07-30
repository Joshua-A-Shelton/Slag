#ifndef SLAG_VULKANBUFFER_H
#define SLAG_VULKANBUFFER_H
#include <vk_mem_alloc.h>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "VulkanGPUMemoryReference.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanBuffer: public Buffer
        {
        public:
            VulkanBuffer(size_t size, Accessibility accessibility,UsageFlags usage);
            VulkanBuffer(void* data, size_t size, Accessibility accessibility,UsageFlags usage);
            ~VulkanBuffer()override;
            VulkanBuffer(const VulkanBuffer&) = delete;
            VulkanBuffer& operator=(const VulkanBuffer&) = delete;
            VulkanBuffer(VulkanBuffer&& from);
            VulkanBuffer& operator=(VulkanBuffer&& from);

            virtual Accessibility accessibility()override;
            virtual uint64_t size()override;
            virtual UsageFlags usage()override;
            virtual void update(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, size_t waitCount,SemaphoreValue* signal, size_t signalCount)override;
            virtual void* cpuHandle()override;

            VkBuffer vulkanHandle() const;
        protected:
            void move(VulkanBuffer& from);
        private:
            VkBuffer _buffer=nullptr;
            uint64_t _size=0;
            VmaAllocation _allocation=nullptr;
            Accessibility _accessibility=Accessibility::GPU;
            UsageFlags _usageFlags=UsageFlags::DATA_BUFFER;
            void* _memoryLocation=nullptr;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::MemoryType::BUFFER, .reference = {.buffer = this}};

            void cpuUpdate(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, size_t waitCount,SemaphoreValue* signal, size_t signalCount);
            void gpuUpdate(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, size_t waitCount,SemaphoreValue* signal, size_t signalCount);

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBUFFER_H
