#include "VulkanBuffer.h"

#include "VulkanBackend.h"
#include "VulkanCommandBuffer.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(
            VulkanGraphicsCard* card,
            uint64_t size,
            BufferCPUAccess cpuAccess,
            BufferMemoryType shaderAccess)
        {
            SLAG_ASSERT(((shaderAccess == BufferMemoryType::UNIFORM && size%256 == 0) || shaderAccess != BufferMemoryType::UNIFORM) && "Buffers with BufferMemoryType::UNIFORM must be a multiple of 256 bytes in size");
            SLAG_ASSERT(((shaderAccess == BufferMemoryType::UNIFORM && size<= card->memoryProperties().maxUniformBufferSize) || shaderAccess != BufferMemoryType::UNIFORM) && "Buffers with BufferMemoryType::UNIFORM cannot exceed size found in GraphicsCard::memoryProperties::maxUniformBufferSize");
            _size = size;
            _graphicsCard = card;
            _dataBits = static_cast<uint8_t>(shaderAccess);
            _dataBits |= static_cast<uint8_t>(cpuAccess) << VKBUFFER_CPU_SHIFT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            //buffer is automatically created on the GPU if there's no cpu access
            if (cpuAccess == BufferCPUAccess::NONE)
            {
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }
            if (cpuAccess == BufferCPUAccess::READ_WRITE)
            {
                allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            }
            if (cpuAccess != BufferCPUAccess::NONE)
            {
                //buffer is visible and consistent from the CPU
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
            if (cpuAccess == BufferCPUAccess::READ_WRITE)
            {
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            }

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = _size;
            bufferCreateInfo.usage = VulkanBackend::nativeBufferUsage(shaderAccess);

            auto result = vmaCreateBuffer(_graphicsCard->allocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw ResourceCreationError("failed to create buffer");
            }
            if (cpuAccess != BufferCPUAccess::NONE)
            {
                vmaMapMemory(_graphicsCard->allocator(),_allocation,&_data);
            }

            vmaSetAllocationUserData(_graphicsCard->allocator(),_allocation,&_selfReference);
        }

        VulkanBuffer::~VulkanBuffer()
        {
            if (_allocation)
            {
                if (VulkanBuffer::cpuAccess() != BufferCPUAccess::NONE)
                {
                    vmaUnmapMemory(_graphicsCard->allocator(),_allocation);
                }
                vmaDestroyBuffer(_graphicsCard->allocator(),_buffer,_allocation);
            }
        }

        VulkanBuffer::VulkanBuffer(VulkanBuffer&& from) noexcept
        {
            move(from);
        }

        VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& from) noexcept
        {
            move(from);
            return *this;
        }

        void* VulkanBuffer::data() const
        {
            SLAG_ASSERT(cpuAccess()!=BufferCPUAccess::NONE && "Attempted to access cpu handle of inaccessible buffer");
            return _data;
        }

        uint64_t VulkanBuffer::deviceAddress() const
        {
            VkBufferDeviceAddressInfoKHR address_info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
            address_info.buffer = _buffer;
            return vkGetBufferDeviceAddress(_graphicsCard->device(), &address_info);
        }

        BufferMemoryType VulkanBuffer::memoryType() const
        {
            return static_cast<BufferMemoryType>((_dataBits & VKBUFFER_MEMORY_BITS));
        }

        BufferCPUAccess VulkanBuffer::cpuAccess() const
        {
            return static_cast<BufferCPUAccess>((_dataBits & VKBUFFER_CPU_BITS) >> VKBUFFER_CPU_SHIFT);
        }

        uint64_t VulkanBuffer::size() const
        {
            return _size;
        }

        GraphicsCard* VulkanBuffer::graphicsCard() const
        {
            return _graphicsCard;
        }

        void* VulkanBuffer::userData()
        {
            return _userData;
        }

        void VulkanBuffer::setUserData(void* userData)
        {
            _userData = userData;
        }

        VkBuffer VulkanBuffer::vulkanHandle() const
        {
            return _buffer;
        }

        VulkanBufferMoveData VulkanBuffer::moveMemory(VmaAllocation tempAllocation, CommandBuffer* copyDataBuffer)
        {
            VkBuffer buffer;
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = _size;
            //every buffer should support transfer
            bufferInfo.usage = VulkanBackend::nativeBufferUsage(this->memoryType());
            auto result = vkCreateBuffer(_graphicsCard->device(),&bufferInfo,nullptr,&buffer);
            if (result != VK_SUCCESS)
            {
                vkDestroyBuffer(_graphicsCard->device(),buffer,nullptr);
                return VulkanBufferMoveData{false,nullptr};
            }
            result = vmaBindBufferMemory(_graphicsCard->allocator(),tempAllocation,buffer);
            if (result != VK_SUCCESS)
            {
                vkDestroyBuffer(_graphicsCard->device(),buffer,nullptr);
                return VulkanBufferMoveData{false,nullptr};
            }
            auto b = static_cast<VulkanCommandBuffer*>(copyDataBuffer)->vulkanHandle();
            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = _size;
            vkCmdCopyBuffer(b,_buffer,buffer,1,&copyRegion);

            VulkanBufferMoveData moveData = {true,_buffer};
            _buffer = buffer;
            return moveData;
        }

        void VulkanBuffer::updatePointer()
        {
            if (_data)
            {
                VmaAllocationInfo allocationInfo = {};
                vmaGetAllocationInfo(_graphicsCard->allocator(),_allocation,&allocationInfo);
                _data = allocationInfo.pMappedData;
            }
        }

        void VulkanBuffer::move(VulkanBuffer& from)
        {
            _size = from._size;
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            std::swap(_data,from._data);
            std::swap(_userData,from._userData);
            _dataBits = from._dataBits;

            if (_allocation)
            {
                vmaSetAllocationUserData(_graphicsCard->allocator(),_allocation,&_selfReference);
            }
        }
    } // vulkan
} // slag
