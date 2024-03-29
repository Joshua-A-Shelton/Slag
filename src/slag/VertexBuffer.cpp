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
                    buffer = new vulkan::VulkanCPUVertexBuffer(data,dataLength, false);
                }
                else
                {
                    buffer = new vulkan::VulkanGPUVertexBuffer(data,dataLength,false);
                }
                break;
            case BackEnd::DX12:
                break;
        }
        return buffer;
    }

    VertexBuffer *VertexBuffer::create(size_t size, Buffer::Usage usage)
    {
        std::vector<unsigned char> empty(size,0);
        return create(empty.data(),size,usage);
    }
} // slag