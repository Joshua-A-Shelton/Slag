#ifndef SLAG_VULKANINDEXBUFFER_H
#define SLAG_VULKANINDEXBUFFER_H

#include "VulkanBuffer.h"
#include "slag/IndexBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanIndexBuffer:public IndexBuffer, public virtual VulkanBuffer
        {
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANINDEXBUFFER_H