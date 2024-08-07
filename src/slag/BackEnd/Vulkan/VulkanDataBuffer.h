#ifndef CRUCIBLEEDITOR_VULKANDATABUFFER_H
#define CRUCIBLEEDITOR_VULKANDATABUFFER_H

#include"../../DataBuffer.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanDataBuffer: public DataBuffer, public virtual VulkanBuffer
        {

        };
    }
}
#endif //CRUCIBLEEDITOR_VULKANDATABUFFER_H