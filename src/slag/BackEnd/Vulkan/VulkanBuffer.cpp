#include "VulkanBuffer.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(void* data, size_t bufferSize, Type type, Buffer::Usage usage)
        {
            _usage = usage;
            //allocate vertex buffer
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            //this is the total size, in bytes, of the buffer we are allocating
            bufferInfo.size = bufferSize;
            switch (type)
            {
                case Type::VERTEX:
                    //this buffer is going to be used as a Vertex VertexBuffer
                    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                    break;
                case Type::INDEX:
                    //this buffer is going to be used as an index buffer
                    bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                    break;
                case Type::STORAGE:
                    //this buffer is going to be used as a Vertex VertexBuffer
                    bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    break;
            }
            _type = type;


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
                case Usage::CPU_TO_GPU:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
                    break;
                case Usage::GPU_TO_CPU:
                    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
                    break;
            }

            //allocate the buffer
            auto result = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(), &bufferInfo, &vmaallocInfo,&_buffer,&_allocation,nullptr);

            assert(result == VK_SUCCESS && "Unable to allocate buffer");

            //directly copy the data
            if(vmaallocInfo.usage == VMA_MEMORY_USAGE_CPU_TO_GPU)
            {
                result = vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(), _allocation, &_mappedLocation);
                assert(result == VK_SUCCESS && "Unable to map buffer memory");

                memcpy(_mappedLocation, data, bufferSize);
            }
            //create temp buffer and transfer
            else if(vmaallocInfo.usage == VMA_MEMORY_USAGE_GPU_ONLY)
            {
                VkBuffer buffer;
                VmaAllocation tempAllocation;

                VkBufferCreateInfo bufferCreateInfo{};
                bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferCreateInfo.size = bufferSize;
                bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

                VmaAllocationCreateInfo allocationCreateInfo{};
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

                auto success = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&buffer,&tempAllocation, nullptr);

                //copy data into temp resources
                void* data;
                vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation,&data);
                memcpy(data,data,static_cast<size_t>(bufferSize));
                vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation);
                VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer commandBuffer)
                {
                    VkBufferCopy copy{.srcOffset=0,.dstOffset=0, .size=bufferSize};
                    vkCmdCopyBuffer(commandBuffer,buffer,_buffer,1,&copy);
                });

                vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,tempAllocation);
            }
            else
            {
                assert(false && "Not Implemented");
            }

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

        VulkanBuffer::VulkanBuffer(VulkanBuffer&& from): Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& from)
        {
            Resource::operator=(std::move(from));
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

        Buffer::Type VulkanBuffer::type()
        {
            return _type;
        }
    } // slag
} // vulkan