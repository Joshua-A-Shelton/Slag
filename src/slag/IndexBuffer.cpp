#include "IndexBuffer.h"
#include "SlagLib.h"
#include "slag/BackEnd/Vulkan/VulkanCPUIndexBuffer.h"
#include "slag/BackEnd/Vulkan/VulkanGPUIndexBuffer.h"

namespace slag
{
    IndexBuffer* IndexBuffer::create(void* data, size_t dataLength, Buffer::Usage usage)
    {
        IndexBuffer* buffer = nullptr;
        switch (SlagLib::usingBackEnd())
        {
            case BackEnd::VULKAN:
                if(usage == Usage::CPU)
                {
                    buffer = new vulkan::VulkanCPUIndexBuffer(data,dataLength, false);
                }
                else
                {
                    buffer = new vulkan::VulkanGPUIndexBuffer(data,dataLength, false);
                }
                break;
            case BackEnd::DX12:
                break;
        }
        return buffer;
    }
} // slag