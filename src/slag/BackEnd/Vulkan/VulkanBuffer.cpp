#include "VulkanBuffer.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(void* data, size_t bufferSize, Buffer::Usage usage)
        {
            _usage = usage;
            //allocate vertex buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            //this is the total size, in bytes, of the buffer we are allocating
            bufferInfo.size = bufferSize;
            //this buffer is going to be used as a Vertex VertexBuffer
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
            VmaAllocationCreateInfo vmaallocInfo = {};
            switch (usage)
            {
                case Usage::CPU_ONLY:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
                    break;
                case Usage::GPU_ONLY:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
                    break;
                case Usage::GPU_TO_CPU:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                    break;
                case Usage::CPU_TO_GPU:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                    break;
            }

            //allocate the buffer
            auto result = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(), &bufferInfo, &vmaallocInfo,&_buffer,&_allocation,nullptr);

            assert(result == VK_SUCCESS && "Unable to allocate buffer");

            vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),_allocation,&_mappedLocation);
            memcpy(_mappedLocation,data,bufferSize);

            auto allocation = _allocation;
            auto buffer = _buffer;
            freeResources= [=]()
            {
                vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),allocation);
                vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,allocation);
            };

        }

        VulkanBuffer::~VulkanBuffer()
        {
            if(_buffer)
            {
                smartDestroy();
            }
        }

        VulkanBuffer::VulkanBuffer(VulkanBuffer&& from)
        {
            move(std::move(from));
        }

        VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanBuffer::move(VulkanBuffer&& from)
        {
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            std::swap(_size,from._size);
            std::swap(_mappedLocation,from._mappedLocation);
        }

        void VulkanBuffer::update(size_t offset, void* data, size_t dataSize)
        {
            assert(offset+dataSize <= _size && "too much data, exceeded buffer size");
            memcpy((char*)(_mappedLocation)+offset,data,dataSize);
        }

        Buffer::Usage VulkanBuffer::usage()
        {
            return Buffer::GPU_ONLY;
        }

        size_t VulkanBuffer::size()
        {
            return _size;
        }

        void* VulkanBuffer::GPUID()
        {
            return _buffer;
        }

        VkBuffer& VulkanBuffer::underlyingBuffer()
        {
            return _buffer;
        }
    } // slag
} // vulkan