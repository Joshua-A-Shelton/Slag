#include "VulkanBuffer.h"
#include "VulkanLib.h"
#include "VulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(void* data, size_t dataLength, Accessibility accessibility,VkBufferUsageFlags usageFlags, bool destroyImmediately): VulkanBuffer(dataLength,accessibility,usageFlags,destroyImmediately)
        {
            VulkanBuffer::update(0,data,dataLength);
        }

        VulkanBuffer::VulkanBuffer(size_t bufferSize, Buffer::Accessibility accessibility, VkBufferUsageFlags usageFlags, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            if(usageFlags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            {
                _usage = _usage | Usage::STORAGE_BUFFER;
            }
            if(usageFlags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT )
            {
                _usage = _usage | Usage::INDEX_BUFFER;
            }
            if(usageFlags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT )
            {
                _usage = _usage | Usage::VERTEX_BUFFER;
            }
            if(usageFlags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT )
            {
                _usage = _usage | Usage::INDIRECT_BUFFER;
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
            //every buffer should support transfer
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

        std::vector<std::byte> VulkanBuffer::downloadData()
        {
            if(_accessibility & CPU)
            {
                return downloadDataCPU();
            }
            else
            {
                return downloadDataGPU();
            }
        }

        void VulkanBuffer::updateCPU(size_t offset, void* data, size_t dataLength)
        {
            memcpy(((char*)_memoryLocation)+offset,data,dataLength);
        }

        void VulkanBuffer::updateGPU(size_t offset, void* data, size_t dataLength)
        {
            VulkanBuffer transfer(data,dataLength,Accessibility::CPU_AND_GPU,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->transferQueueFamily());
            commandBuffer.begin();
            commandBuffer.copyBuffer(&transfer,0,dataLength,this,offset);
            commandBuffer.end();
            VulkanLib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        std::vector<std::byte> VulkanBuffer::downloadDataCPU()
        {
            std::vector<std::byte> bytes(_size);
            memcpy(bytes.data(),_memoryLocation,_size);
            return bytes;
        }

        std::vector<std::byte> VulkanBuffer::downloadDataGPU()
        {
            VulkanBuffer temp(_size,Accessibility::CPU_AND_GPU,VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
            VulkanCommandBuffer commandBuffer(VulkanLib::card()->transferQueueFamily());
            commandBuffer.begin();
            commandBuffer.copyBuffer(this,0,_size,&temp,0);
            commandBuffer.end();
            VulkanLib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();

            return temp.downloadDataCPU();
        }

        size_t VulkanBuffer::size()
        {
            return _size;
        }

        Buffer::Accessibility VulkanBuffer::accessibility()
        {
            return _accessibility;
        }

        unsigned char* VulkanBuffer::cpuHandle()
        {
            return static_cast<unsigned char*>(_memoryLocation);
        }

        VkBuffer VulkanBuffer::underlyingBuffer()
        {
            return _buffer;
        }

    } // vulkan
} // slag