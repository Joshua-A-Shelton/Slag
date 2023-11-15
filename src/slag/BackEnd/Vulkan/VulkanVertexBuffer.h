#ifndef SLAG_VULKANVERTEXBUFFER_H
#define SLAG_VULKANVERTEXBUFFER_H
#include "../../VertexBuffer.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanVertexBuffer: public VertexBuffer, public virtual VulkanBuffer
        {
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANVERTEXBUFFER_H