#include "VulkanSamplerDescriptorMemory.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSampler.h"
#include "slag/backends/vulkan/VulkanExtensions.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSamplerDescriptorMemory::VulkanSamplerDescriptorMemory(uint64_t descriptorCount)
        {
            _descriptorSetAlignment = VulkanGraphicsCard::selected()->descriptorBufferOffsetAlignment();
            _size = descriptorCount * VulkanGraphicsCard::selected()->samplerDescriptorSize();

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
            //buffer is created on the gpu
            allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            //buffer is visible and consistent from the CPU
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            allocationCreateInfo.pUserData = &_selfReference;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = _size;
            bufferCreateInfo.usage =  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

            auto result = vmaCreateBuffer(VulkanGraphicsCard::selected()->allocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer");
            }
            vmaMapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation,&_bufferPointer);
        }

        VulkanSamplerDescriptorMemory::~VulkanSamplerDescriptorMemory()
        {
            if (_allocation)
            {
                vmaUnmapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation);
                vmaDestroyBuffer(VulkanGraphicsCard::selected()->allocator(),_buffer,_allocation);
            }
        }

        VulkanSamplerDescriptorMemory::VulkanSamplerDescriptorMemory(VulkanSamplerDescriptorMemory&& from)
        {
            move(from);
        }

        VulkanSamplerDescriptorMemory& VulkanSamplerDescriptorMemory::operator=(VulkanSamplerDescriptorMemory&& from)
        {
            move(from);
            return *this;
        }

        uint64_t VulkanSamplerDescriptorMemory::size()
        {
            return _size;
        }

        uint64_t VulkanSamplerDescriptorMemory::handle()
        {
            return deviceAddress();
        }

        uint64_t VulkanSamplerDescriptorMemory::nextDescriptorGroupOffset(uint64_t memoryLocation)
        {
            if (_descriptorSetAlignment == 0)
            {
                return memoryLocation;
            }
            return ( ( memoryLocation - 1 ) | ( _descriptorSetAlignment - 1 ) ) + 1;
        }

        void VulkanSamplerDescriptorMemory::setSampler(uint64_t memoryLocation, Sampler* sampler)
        {
            VulkanSampler* vulkanSampler = static_cast<VulkanSampler*>(sampler);
            auto passSampler = vulkanSampler->vulkanHandle();
            VkDescriptorGetInfoEXT textureDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLER,
                .data = {.pSampler = &passSampler}
            };
            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&textureDescriptorInfo,card->samplerDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        VulkanBufferMoveData VulkanSamplerDescriptorMemory::moveMemory(VmaAllocation tempAllocation, CommandBuffer* copyDataBuffer)
        {
            VkBuffer buffer;
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = _size;
            //every buffer should support transfer
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
            auto result = vkCreateBuffer(VulkanGraphicsCard::selected()->device(),&bufferInfo,nullptr,&buffer);
            if (result != VK_SUCCESS)
            {
                vkDestroyBuffer(VulkanGraphicsCard::selected()->device(),buffer,nullptr);
                return VulkanBufferMoveData{false,nullptr};
            }
            result = vmaBindBufferMemory(VulkanGraphicsCard::selected()->allocator(),tempAllocation,buffer);
            if (result != VK_SUCCESS)
            {
                vkDestroyBuffer(VulkanGraphicsCard::selected()->device(),buffer,nullptr);
                return VulkanBufferMoveData{false,nullptr};
            }
            auto b = static_cast<VulkanCommandBuffer*>(copyDataBuffer)->vulkanCommandBufferHandle();
            VkBufferCopy copyRegion = {};
            copyRegion.srcOffset = 0;
            copyRegion.dstOffset = 0;
            copyRegion.size = _size;
            vkCmdCopyBuffer(b,_buffer,buffer,1,&copyRegion);

            VulkanBufferMoveData moveData = {true,_buffer};
            _buffer = buffer;
            return moveData;
        }

        VkBuffer VulkanSamplerDescriptorMemory::vulkanBuffer() const
        {
            return _buffer;
        }

        VkDeviceAddress VulkanSamplerDescriptorMemory::deviceAddress() const
        {
            VkBufferDeviceAddressInfo deviceAdressInfo{};
            deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            deviceAdressInfo.buffer = _buffer;
            uint64_t address = vkGetBufferDeviceAddress(VulkanGraphicsCard::selected()->device(), &deviceAdressInfo);
            return address;
        }

        void VulkanSamplerDescriptorMemory::move(VulkanSamplerDescriptorMemory& from)
        {
            std::swap(_buffer, from._buffer);
            std::swap(_allocation,from._allocation);
            _size = from._size;
            _descriptorSetAlignment = from._descriptorSetAlignment;
            std::swap(_bufferPointer, from._bufferPointer);
            if(_allocation)
            {
                vmaSetAllocationUserData(VulkanGraphicsCard::selected()->allocator(),_allocation,&_selfReference);
            }
        }
    } // vulkan
} // slag
