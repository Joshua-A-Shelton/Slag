#include "VertexBuffer.h"
#include "SlagLib.h"
#include "BackEnd/Vulkan/VulkanCPUVertexBuffer.h"
#include "slag/BackEnd/Vulkan/VulkanGPUVertexBuffer.h"

namespace slag
{
    VertexBuffer* VertexBuffer::create(void* data, size_t dataLength, Buffer::Usage usage)
    {
        VertexBuffer* buffer = nullptr;
        switch (SlagLib::usingBackEnd())
        {
            case BackEnd::VULKAN:
                if(usage == Usage::CPU)
                {
                    buffer = new vulkan::VulkanCPUVertexBuffer(data,dataLength);
                }
                else
                {
                    buffer = new vulkan::VulkanGPUVertexBuffer(data,dataLength);
                }
                break;
            case BackEnd::DX12:
                break;
        }
        return buffer;
    }
} // slag