#include "VulkanResourceDescriptorMemory.h"

#include <any>

#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanTexture.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/backends/vulkan/VulkanExtensions.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanResourceDescriptorMemory::VulkanResourceDescriptorMemory(uint64_t descriptorCount)
        {
            _descriptorSetAlignment = VulkanGraphicsCard::selected()->descriptorBufferOffsetAlignment();
            _size = descriptorCount * VulkanGraphicsCard::selected()->maxResourceDescriptorSize();

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
            bufferCreateInfo.usage =  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

            auto result = vmaCreateBuffer(VulkanGraphicsCard::selected()->allocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer");
            }
            vmaMapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation,&_bufferPointer);
        }

        VulkanResourceDescriptorMemory::~VulkanResourceDescriptorMemory()
        {
            if (_allocation)
            {
                vmaUnmapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation);
                vmaDestroyBuffer(VulkanGraphicsCard::selected()->allocator(),_buffer,_allocation);
            }
        }

        VulkanResourceDescriptorMemory::VulkanResourceDescriptorMemory(VulkanResourceDescriptorMemory&& from)
        {
            move(from);
        }

        VulkanResourceDescriptorMemory& VulkanResourceDescriptorMemory::operator=(VulkanResourceDescriptorMemory&& from)
        {
            move(from);
            return *this;
        }

        uint64_t VulkanResourceDescriptorMemory::size()
        {
            return _size;
        }

        uint64_t VulkanResourceDescriptorMemory::handle()
        {
            return deviceAddress();
        }

        uint64_t VulkanResourceDescriptorMemory::nextDescriptorGroupOffset(uint64_t memoryLocation)
        {
            if (_descriptorSetAlignment == 0)
            {
                return memoryLocation;
            }
            auto offAlign = memoryLocation % _descriptorSetAlignment;
            if (offAlign == 0)
            {
                return memoryLocation;
            }
            return memoryLocation + (_descriptorSetAlignment - (offAlign));
        }

        void VulkanResourceDescriptorMemory::setSampledTexture(uint64_t memoryLocation, Texture* texture)
        {
            SLAG_ASSERT((texture->usageFlags() & Texture::UsageFlags::SAMPLED_IMAGE) == Texture::UsageFlags::SAMPLED_IMAGE && "Given texture is not a sampled texture");
            VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkDescriptorImageInfo descriptorImageInfo{};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            descriptorImageInfo.imageView = vulkanTexture->vulkanViewHandle();
            VkDescriptorGetInfoEXT textureDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                .data = {.pSampledImage = &descriptorImageInfo}
            };
            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&textureDescriptorInfo,card->sampledTextureDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        void VulkanResourceDescriptorMemory::setStorageTexture(uint64_t memoryLocation, Texture* texture)
        {
            SLAG_ASSERT((texture->usageFlags() & Texture::UsageFlags::STORAGE) == Texture::UsageFlags::STORAGE && "Given texture is not a storage texture");
            VulkanTexture* vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkDescriptorImageInfo descriptorImageInfo{};
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            descriptorImageInfo.imageView = vulkanTexture->vulkanViewHandle();
            VkDescriptorGetInfoEXT textureDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .data = {.pStorageImage = &descriptorImageInfo}
            };
            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&textureDescriptorInfo,card->storageTextureDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        void VulkanResourceDescriptorMemory::setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER) == Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER && "Given buffer is not a uniform texel buffer");
            SLAG_ASSERT(Pixels::AspectFlags(format) == Pixels::AspectFlags::COLOR && "Only color formats can be bound as texel buffer");
            auto size = Pixels::size(format);
            SLAG_ASSERT((startIndex + elementCount)*size <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            VulkanBuffer* vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkDescriptorAddressInfoEXT addressInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext = nullptr,
                .address = vulkanBuffer->deviceAddress() + (startIndex*size),
                .range = elementCount*size,
                .format = VulkanBackend::vulkanizedFormat(format).format
            };

            VkDescriptorGetInfoEXT bufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
                .data = {.pUniformTexelBuffer = &addressInfo}
            };

            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&bufferDescriptorInfo,card->uniformTexelBufferDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        void VulkanResourceDescriptorMemory::setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::STORAGE_TEXEL_BUFFER) == Buffer::UsageFlags::STORAGE_TEXEL_BUFFER && "Given buffer is not a storage texel buffer");
            SLAG_ASSERT(Pixels::AspectFlags(format) == Pixels::AspectFlags::COLOR && "Only color formats can be bound as texel buffer");
            auto size = Pixels::size(format);
            SLAG_ASSERT((startIndex + elementCount)*size <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            VulkanBuffer* vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkDescriptorAddressInfoEXT addressInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext = nullptr,
                .address = vulkanBuffer->deviceAddress() + startIndex*size,
                .range = elementCount * size,
                .format = VulkanBackend::vulkanizedFormat(format).format
            };

            VkDescriptorGetInfoEXT bufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                .data = {.pStorageTexelBuffer = &addressInfo}
            };

            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&bufferDescriptorInfo,card->storageTexelBufferDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        void VulkanResourceDescriptorMemory::setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::UNIFORM_BUFFER) == Buffer::UsageFlags::UNIFORM_BUFFER && "Given buffer is not a uniform buffer");
            SLAG_ASSERT((startIndex + elementCount)*dataStride <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            VulkanBuffer* vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkDescriptorAddressInfoEXT addressInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext = nullptr,
                .address = vulkanBuffer->deviceAddress() + (startIndex*dataStride),
                .range = elementCount*dataStride,
                .format = VK_FORMAT_UNDEFINED
            };

            VkDescriptorGetInfoEXT bufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .data = {.pUniformBuffer = &addressInfo}
            };

            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&bufferDescriptorInfo,card->uniformBufferDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        void VulkanResourceDescriptorMemory::setStorageBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::STORAGE_BUFFER) == Buffer::UsageFlags::STORAGE_BUFFER && "Given buffer is not a storage buffer");
            SLAG_ASSERT((startIndex + elementCount)*dataStride <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            VulkanBuffer* vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkDescriptorAddressInfoEXT addressInfo =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
                .pNext = nullptr,
                .address = vulkanBuffer->deviceAddress() + (startIndex*dataStride),
                .range = elementCount*dataStride,
                .format = VK_FORMAT_UNDEFINED
            };

            VkDescriptorGetInfoEXT bufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                .data = {.pStorageBuffer = &addressInfo}
            };

            auto card = VulkanGraphicsCard::selected();
            slag_vkGetDescriptorEXT(card->device(),&bufferDescriptorInfo,card->storageBufferDescriptorSize(),static_cast<unsigned char*>(_bufferPointer)+memoryLocation);
        }

        VulkanBufferMoveData VulkanResourceDescriptorMemory::moveMemory(VmaAllocation tempAllocation,CommandBuffer* copyDataBuffer)
        {
            VkBuffer buffer;
            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = _size;
            //every buffer should support transfer
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;
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

        VkBuffer VulkanResourceDescriptorMemory::vulkanBuffer() const
        {
            return _buffer;
        }

        VkDeviceAddress VulkanResourceDescriptorMemory::deviceAddress() const
        {
            VkBufferDeviceAddressInfo deviceAdressInfo{};
            deviceAdressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            deviceAdressInfo.buffer = _buffer;
            uint64_t address = vkGetBufferDeviceAddress(VulkanGraphicsCard::selected()->device(), &deviceAdressInfo);
            return address;
        }

        void VulkanResourceDescriptorMemory::move(VulkanResourceDescriptorMemory& from)
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
