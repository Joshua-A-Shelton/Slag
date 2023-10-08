#ifndef SLAG_VULKANUNIFORMBUFFER_H
#define SLAG_VULKANUNIFORMBUFFER_H
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <optional>
#include "../Resource.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanUniformBuffer: public Resource
        {
        private:
            VkBuffer _backingBuffer = nullptr;
            VmaAllocation _allocation = nullptr;
            void* _memoryLocation = nullptr;
            VkDeviceSize _size = 0;
            size_t _offset = 0;
            size_t _minUniformBufferOffsetAlignment=0;
            VulkanUniformBuffer(VkDeviceSize defaultSize, bool destroyImmediate);
            void _create(VkDeviceSize size);
            void _destroy();
            void move(VulkanUniformBuffer&& from);
            size_t paddedSize(size_t originalSize);
        public:
            ~VulkanUniformBuffer();
            VulkanUniformBuffer()=delete;
            VulkanUniformBuffer(const VulkanUniformBuffer&)=delete;
            VulkanUniformBuffer& operator=(const VulkanUniformBuffer&)=delete;
            VulkanUniformBuffer(VulkanUniformBuffer&& from);
            VulkanUniformBuffer& operator=(VulkanUniformBuffer&& from);
            VkDeviceSize size();

            void reset();
            void reset(VkDeviceSize newSize);

            std::optional<size_t> write(void* data, size_t dataSize);
            VkBuffer backingBuffer();
            void* GPUID()override;
            friend class VulkanVirtualUniformBuffer;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANUNIFORMBUFFER_H