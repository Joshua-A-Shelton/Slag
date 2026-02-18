#include "VulkanBuffer.h"

#include "VulkanBackend.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(
            VulkanGraphicsCard* card,
            uint64_t size,
            BufferUsage usage,
            BufferShaderAccess shaderAccess,
            BufferCPUAccess cpuAccess)
        {
            SLAG_ASSERT(((shaderAccess == BufferShaderAccess::READ_ONLY && size%256 == 0) || shaderAccess != BufferShaderAccess::READ_ONLY) && "Buffers with BufferShaderAccess::READ_ONLY must be a multiple of 256 bytes in size");
            SLAG_ASSERT(((shaderAccess == BufferShaderAccess::READ_ONLY && size<= card->maxShaderAccessReadOnlyBufferSize()) || shaderAccess != BufferShaderAccess::READ_ONLY) && "Buffers with BufferShaderAccess::READ_ONLY cannot exceed size found in GraphicsCard::maxShaderAccessReadOnlyBufferSize");
            _size = size;
            _graphicsCard = card;
            _dataBits = static_cast<uint8_t>(usage);
            _dataBits |= static_cast<uint8_t>(shaderAccess) << VKBUFFER_SHADER_SHIFT;
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
            bufferCreateInfo.usage = VulkanBackend::nativeBufferUsage(usage,shaderAccess);

            auto result = vmaCreateBuffer(_graphicsCard->allocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw ResourceCreationError("failed to create buffer");
            }
            if (cpuAccess != BufferCPUAccess::NONE)
            {
                vmaMapMemory(_graphicsCard->allocator(),_allocation,&_data);
            }
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

        BufferUsage VulkanBuffer::usage() const
        {
            return static_cast<BufferUsage>(_dataBits & VKBUFFER_USAGE_BITS);
        }

        BufferShaderAccess VulkanBuffer::shaderAccess() const
        {
            return static_cast<BufferShaderAccess>((_dataBits & VKBUFFER_SHADER_BITS) >> VKBUFFER_SHADER_SHIFT);
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

        VkBuffer VulkanBuffer::vulkanHandle() const
        {
            return _buffer;
        }

        void VulkanBuffer::move(VulkanBuffer& from)
        {
            _size = from._size;
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            std::swap(_data,from._data);
            _dataBits = from._dataBits;
        }
    } // vulkan
} // slag
