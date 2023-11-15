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
        class VulkanBuffer: virtual public Buffer
        {
        public:
            virtual VkBuffer underlyingBuffer()=0;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANBUFFER_H