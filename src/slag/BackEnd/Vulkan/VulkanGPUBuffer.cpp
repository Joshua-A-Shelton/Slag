#include "VulkanGPUBuffer.h"
#include "VulkanLib.h"


namespace slag
{
    namespace vulkan
    {
        VulkanGPUBuffer::VulkanGPUBuffer(void* data, size_t dataLength, VkBufferUsageFlags usageFlags)
        {
            _bufferSize = dataLength;
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            //this is the total size, in bytes, of the buffer we are allocating
            bufferInfo.size = dataLength;
            bufferInfo.usage = usageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            VmaAllocationCreateInfo vmaallocInfo = {};
            vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            //allocate the buffer
            auto result = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(), &bufferInfo, &vmaallocInfo,&_buffer,&_allocation,nullptr);

            assert(result == VK_SUCCESS && "Unable to allocate buffer");

            VkBuffer buffer;
            VmaAllocation tempAllocation;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = dataLength;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

            auto success = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&buffer,&tempAllocation, nullptr);

            //copy data into temp resources
            void* dataLocation;
            vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation,&dataLocation);
            memcpy(dataLocation,data,static_cast<size_t>(dataLength));
            vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation);
            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer commandBuffer)
            {
                VkBufferCopy copy{.srcOffset=0,.dstOffset=0, .size=dataLength};
                vkCmdCopyBuffer(commandBuffer,buffer,_buffer,1,&copy);
            });

            vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,tempAllocation);

            auto allocation = _allocation;
            auto buf = _buffer;
            freeResources= [=]()
            {
                vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buf,allocation);
            };
        }

        VulkanGPUBuffer::~VulkanGPUBuffer()
        {
            if(_buffer)
            {
                smartDestroy();
            }
        }

        VulkanGPUBuffer::VulkanGPUBuffer(VulkanGPUBuffer&& from)
        {
            move(from);
        }

        VulkanGPUBuffer& VulkanGPUBuffer::operator=(VulkanGPUBuffer&& from)
        {
            move(from);
            return *this;
        }

        size_t VulkanGPUBuffer::size()
        {
            return _bufferSize;
        }

        void VulkanGPUBuffer::update(size_t offset, void* data, size_t dataLength)
        {
            VkBuffer buffer;
            VmaAllocation tempAllocation;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = dataLength;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

            auto success = vmaCreateBuffer(VulkanLib::graphicsCard()->memoryAllocator(),&bufferCreateInfo,&allocationCreateInfo,&buffer,&tempAllocation, nullptr);

            //copy data into temp resources
            void* dataLocation;
            vmaMapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation,&dataLocation);
            memcpy(dataLocation,data,static_cast<size_t>(dataLength));
            vmaUnmapMemory(VulkanLib::graphicsCard()->memoryAllocator(),tempAllocation);
            VulkanLib::graphicsCard()->runOneTimeCommands(VulkanLib::graphicsCard()->transferQueue(),VulkanLib::graphicsCard()->transferQueueFamily(),[=](VkCommandBuffer commandBuffer)
            {
                VkBufferCopy copy{.srcOffset=0,.dstOffset=offset, .size=dataLength};
                vkCmdCopyBuffer(commandBuffer,buffer,_buffer,1,&copy);
            });

            vmaDestroyBuffer(VulkanLib::graphicsCard()->memoryAllocator(),buffer,tempAllocation);
        }

        void* VulkanGPUBuffer::GPUID()
        {
            return _buffer;
        }

        VkBuffer VulkanGPUBuffer::underlyingBuffer()
        {
            return _buffer;
        }

        void VulkanGPUBuffer::move(VulkanGPUBuffer& from)
        {
            std::swap(_buffer,from._buffer);
            std::swap(_allocation, from._allocation);
            std::swap(_bufferSize,from._bufferSize);
        }

        Buffer::Usage VulkanGPUBuffer::usage()
        {
            return Buffer::GPU;
        }
    } // slag
} // vulkan