#include "VulkanBuffer.h"
#include "VulkanLib.h"
#include "VulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(void* data, size_t dataLength, Accessibility accessibility,VkBufferUsageFlags usageFlags, bool destroyImmediately): VulkanBuffer(dataLength,accessibility,usageFlags,destroyImmediately)
        {
            update(0,data,dataLength);
        }

        VulkanBuffer::VulkanBuffer(size_t bufferSize, Buffer::Accessibility accessibility,VkBufferUsageFlags usageFlags, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            if(usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            {
                _usage = _usage | Usage::Storage;
            }
            if(usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )
            {
                _usage = _usage | Usage::IndexBuffer;
            }
            if(usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )
            {
                _usage = _usage | Usage::VertexBuffer;
            }
            if(usageFlags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT )
            {
                _usage = _usage | Usage::Indirect;
            }

            _accessibility = accessibility;
            _size = bufferSize;
            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

            if(_accessibility & CPU)
            {
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
            if(_accessibility & GPU)
            {
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = bufferSize;
            bufferCreateInfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            auto result = vmaCreateBuffer(VulkanLib::card()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation, nullptr);
            if(result != VK_SUCCESS)
            {
                throw std::runtime_error("unable to create buffer");
            }

            auto buf = _buffer;
            auto alloc = _allocation;
            if(_accessibility & CPU)
            {
                vmaMapMemory(VulkanLib::card()->memoryAllocator(),alloc,&_memoryLocation);
                _disposeFunction = [=]
                {
                    vmaUnmapMemory(VulkanLib::card()->memoryAllocator(),alloc);
                    vmaDestroyBuffer(VulkanLib::card()->memoryAllocator(),buf,alloc);
                };
            }
            else
            {
                _disposeFunction = [=]
                {
                    vmaDestroyBuffer(VulkanLib::card()->memoryAllocator(),buf,alloc);
                };
            }
        }

        VulkanBuffer::~VulkanBuffer()
        {
            if(_buffer)
            {
                smartDestroy();
            }
        }

        VulkanBuffer::VulkanBuffer(VulkanBuffer&& from): resources::Resource(from._destroyImmediately)
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
            resources::Resource::move(from);
            std::swap(_buffer, from._buffer);
            std::swap(_allocation,from._allocation);
            _accessibility = from._accessibility;
            _usage = from._usage;
            _size = from._size;
            std::swap(_memoryLocation, from._memoryLocation);
        }

        void VulkanBuffer::update(size_t offset, void* data, size_t dataLength)
        {
            assert(offset+dataLength <= _size && "update extends beyond bound of buffer");
            if(_accessibility & CPU)
            {
                updateCPU(offset,data,dataLength);
            }
            else
            {
                updateGPU(offset,data,dataLength);
            }
        }

        void VulkanBuffer::updateCPU(size_t offset, void* data, size_t dataLength)
        {
            memcpy(((char*)_memoryLocation)+offset,data,dataLength);
        }

        void VulkanBuffer::updateGPU(size_t offset, void* data, size_t dataLength)
        {
            VkBuffer buffer;
            VmaAllocation tempAllocation;
            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = dataLength;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

            auto success = vmaCreateBuffer(VulkanLib::card()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&buffer,&tempAllocation, nullptr);

            //copy data into temp resources
            void* dataLocation;
            vmaMapMemory(VulkanLib::card()->memoryAllocator(),tempAllocation,&dataLocation);
            memcpy(dataLocation,data,static_cast<size_t>(dataLength));
            vmaUnmapMemory(VulkanLib::card()->memoryAllocator(),tempAllocation);
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->transferQueueFamily());
            commandBuffer.begin();

            VkBufferCopy copy{.srcOffset=0,.dstOffset=offset, .size=dataLength};
            vkCmdCopyBuffer(commandBuffer.underlyingCommandBuffer(),buffer,_buffer,1,&copy);

            commandBuffer.end();
            VulkanLib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();

            vmaDestroyBuffer(VulkanLib::card()->memoryAllocator(),buffer,tempAllocation);
        }

        void* VulkanBuffer::gpuID()
        {
            return _buffer;
        }

        size_t VulkanBuffer::size()
        {
            return _size;
        }

        Buffer::Accessibility VulkanBuffer::accessibility()
        {
            return _accessibility;
        }

        VkBuffer VulkanBuffer::underlyingBuffer()
        {
            return _buffer;
        }

    } // vulkan
} // slag