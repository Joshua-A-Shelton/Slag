#include "VulkanCPUBuffer.h"
#include "VulkanLib.h"


namespace slag
{
    namespace vulkan
    {
        VulkanCPUBuffer::VulkanCPUBuffer(void* data, size_t dataLength, VkBufferUsageFlags usageFlags, bool destroyImmediate)
        {
            destroyImmediately = destroyImmediate;
            build(data,dataLength,usageFlags);
        }

        VulkanCPUBuffer::~VulkanCPUBuffer()
        {
            if(_buffer)
            {
                smartDestroy();
            }
        }

        VulkanCPUBuffer::VulkanCPUBuffer(VulkanCPUBuffer&& from)
        {
            move(from);
        }

        VulkanCPUBuffer& VulkanCPUBuffer::operator=(VulkanCPUBuffer&& from)
        {
            move(from);
            return *this;
        }

        size_t VulkanCPUBuffer::size()
        {
            return _bufferSize;
        }

        void VulkanCPUBuffer::update(size_t offset, void* data, size_t dataLength)
        {
            assert(offset+dataLength <= _bufferSize && "too much data, exceeded buffer size");
            memcpy((char*)(_mappedLocation)+offset,data,dataLength);
        }

        void VulkanCPUBuffer::rebuild(size_t newSize)
        {
            vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),_allocation);
            vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),_buffer,_allocation);

            std::vector<unsigned char> data(newSize,0);
            build(data.data(),data.size(),_flags);
        }

        void* VulkanCPUBuffer::GPUID()
        {
            return _buffer;
        }

        VkBuffer VulkanCPUBuffer::underlyingBuffer()
        {
            return _buffer;
        }

        void VulkanCPUBuffer::move(VulkanCPUBuffer& from)
        {
            std::swap(_mappedLocation,from._mappedLocation);
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            std::swap(_bufferSize,from._bufferSize);
            std::swap(_flags,from._flags);
        }

        Buffer::Usage VulkanCPUBuffer::usage()
        {
            return Buffer::CPU;
        }

        void VulkanCPUBuffer::build(void *data, size_t dataLength, VkBufferUsageFlags usageFlags)
        {
            _bufferSize = dataLength;
            _flags = usageFlags;
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            //this is the total size, in bytes, of the buffer we are allocating
            bufferInfo.size = dataLength;
            bufferInfo.usage = usageFlags;
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            auto result = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(), &bufferInfo, &vmaallocInfo,&_buffer,&_allocation,nullptr);

            assert(result == VK_SUCCESS && "Unable to allocate buffer");

            result = vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(), _allocation, &_mappedLocation);
            assert(result == VK_SUCCESS && "Unable to map buffer memory");

            memcpy(_mappedLocation, data, dataLength);

            auto allocation = _allocation;
            auto buffer = _buffer;
            freeResources= [=]()
            {
                vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),allocation);
                vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,allocation);
            };
        }

    } // slag
} // vulkan