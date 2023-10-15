#include "VulkanUniformBuffer.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniformBuffer::VulkanUniformBuffer(VkDeviceSize defaultSize, bool destroyImmediate)
        {
            destroyImmediately = destroyImmediate;

            _create(defaultSize);
        }

        VulkanUniformBuffer::~VulkanUniformBuffer()
        {
            _destroy();
        }

        void VulkanUniformBuffer::reset()
        {
            _offset = 0;
        }

        void VulkanUniformBuffer::reset(VkDeviceSize newSize)
        {
            _destroy();
            _create(newSize);
            reset();
        }

        void VulkanUniformBuffer::_create(VkDeviceSize size)
        {
            _minUniformBufferOffsetAlignment = VulkanLib::graphicsCard()->properties().limits.minUniformBufferOffsetAlignment;
            _size = paddedSize(size);
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.pNext = nullptr;
            bufferInfo.size = _size;

            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;


            //let the VMA library know that this data should be writeable by CPU, but also readable by GPU
            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

            //allocate the buffer
            vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(), &bufferInfo, &vmaallocInfo,
                            &_backingBuffer,
                            &_allocation,
                            nullptr);
            vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),_allocation,&_memoryLocation);

            auto alloc = _allocation;
            auto back = _backingBuffer;
            freeResources = [=]()
            {
                vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),alloc);
                vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(), back, alloc);
            };
        }

        void VulkanUniformBuffer::_destroy()
        {
            if(_allocation)
            {
                smartDestroy();
            }
        }

        size_t VulkanUniformBuffer::paddedSize(size_t originalSize)
        {
            // Calculate required alignment based on minimum device offset alignment
            size_t alignedSize = originalSize;
            if (_minUniformBufferOffsetAlignment > 0)
            {
                alignedSize = (alignedSize + _minUniformBufferOffsetAlignment - 1) & ~(_minUniformBufferOffsetAlignment - 1);
            }
            return alignedSize;
        }

        VulkanUniformBuffer::VulkanUniformBuffer(VulkanUniformBuffer &&from): Resource(std::move(from))
        {
            move(std::move(from));
        }

        VulkanUniformBuffer &VulkanUniformBuffer::operator=(VulkanUniformBuffer &&from)
        {
            Resource::operator=(std::move(from));
            move(std::move(from));
            return *this;
        }

        VkDeviceSize VulkanUniformBuffer::size()
        {
            return _size;
        }


        std::optional<size_t> VulkanUniformBuffer::write(void *data, size_t dataSize)
        {
            if(_offset + dataSize > _size)
            {
                return std::optional<size_t>();
            }
            size_t startWrite = _offset;
            memcpy(static_cast<char*>(_memoryLocation)+_offset,data,dataSize);
            _offset += paddedSize(dataSize);
            return std::optional<size_t>(startWrite);
        }

        VkBuffer VulkanUniformBuffer::backingBuffer()
        {
            return _backingBuffer;
        }

        void* VulkanUniformBuffer::GPUID()
        {
            return _backingBuffer;
        }

        void VulkanUniformBuffer::move(VulkanUniformBuffer&& from)
        {
            _backingBuffer = from._backingBuffer;
            _allocation = from._allocation;
            _memoryLocation = from._memoryLocation;
            _size = from._size;
            _offset = from._offset;
            _minUniformBufferOffsetAlignment=from._minUniformBufferOffsetAlignment;

            from._backingBuffer = nullptr;
            from._allocation = nullptr;
            from._memoryLocation = nullptr;
            from._size = 0;
            from._offset = 0;
            from.freeResources = nullptr;
        }
    } // slag
} // vulkan