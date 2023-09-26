#ifndef SLAG_VULKANVIRTUALUNIFORMBUFFER_H
#define SLAG_VULKANVIRTUALUNIFORMBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanUniformBuffer.h"
#include "VulkanDescriptorAllocator.h"
namespace slag
{
    namespace vulkan
    {
        struct VulkanUniformBufferWriteData
        {
        public:
            VkBuffer buffer;
            VkDeviceSize startingOffset;
            VkDeviceSize writtenSize;
        };

        class VulkanVirtualUniformBuffer
        {
        private:
            VkDeviceSize _virtualSize=0;
            std::vector<VulkanUniformBuffer> _backingBuffers;
            size_t _currentBufferIndex = 0;
        public:
            explicit VulkanVirtualUniformBuffer(VkDeviceSize defaultSize = 1250000);
            VulkanUniformBufferWriteData write(void* data, size_t size);
            void reset();
            VkDeviceSize virtualSize();
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANVIRTUALUNIFORMBUFFER_H