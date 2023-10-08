#ifndef SLAG_VULKANVIRTUALUNIFORMBUFFER_H
#define SLAG_VULKANVIRTUALUNIFORMBUFFER_H
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanUniformBuffer.h"
#include "VulkanDescriptorAllocator.h"
#include "../../UniformBuffer.h"
namespace slag
{
    namespace vulkan
    {

        class VulkanVirtualUniformBuffer: public UniformBuffer
        {
        private:
            uint64_t _virtualSize=0;
            std::vector<VulkanUniformBuffer> _backingBuffers;
            size_t _currentBufferIndex = 0;
            bool _destroyImmediately = false;
            void move(VulkanVirtualUniformBuffer&& from);
        public:
            explicit VulkanVirtualUniformBuffer(VkDeviceSize defaultSize,bool destroyImmediate);
            VulkanVirtualUniformBuffer(const VulkanUniformBuffer&)=delete;
            VulkanVirtualUniformBuffer& operator=(const VulkanUniformBuffer&)=delete;
            VulkanVirtualUniformBuffer(VulkanVirtualUniformBuffer&& from);
            VulkanVirtualUniformBuffer& operator=(VulkanVirtualUniformBuffer&& from);
            WriteLocation write(void* data, size_t size);
            void reset()override;
            uint64_t virtualSize()override;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANVIRTUALUNIFORMBUFFER_H