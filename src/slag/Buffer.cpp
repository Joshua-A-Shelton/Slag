#include "Buffer.h"
#include "BackEnd/Vulkan/VulkanBuffer.h"
#include "SlagLib.h"
namespace slag
{

    Buffer* Buffer::create(void* data, size_t bufferSize, Usage usage)
    {
        switch (SlagLib::usingBackEnd())
        {
            case VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                return new vulkan::VulkanBuffer(data,bufferSize,usage);
#endif
                break;
        }
        return nullptr;
    }
}