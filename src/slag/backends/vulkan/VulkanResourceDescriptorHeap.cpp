#include "VulkanResourceDescriptorHeap.h"

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanTexture.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanResourceDescriptorHeap::VulkanResourceDescriptorHeap(VulkanGraphicsCard* card, uint32_t descriptorCount)
        {
            _card = card;
            auto heapDetails = _card->descriptorHeapDetails();
            _descriptorCount = descriptorCount;
            _descriptorSize = heapDetails.resourceDescriptorIncrementSize;

            SLAG_ASSERT(descriptorCount > 0 && "Descriptor count must be greater than 0");
            SLAG_ASSERT(descriptorCount <= heapDetails.maxResourceDescriptors && "Exceeded max heap size");

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = (descriptorCount * heapDetails.resourceDescriptorIncrementSize) + heapDetails.resourceReservedRangeSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            _reserved = heapDetails.resourceReservedRangeSize;

            auto result = vmaCreateBufferWithAlignment(_card->allocator(),&bufferCreateInfo,&allocationCreateInfo,32,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw ResourceCreationError("failed to create buffer");
            }
            vmaMapMemory(_card->allocator(),_allocation,&_data);

            VkBufferDeviceAddressInfo bufferDeviceAddressInfo
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = _buffer,
            };
            _deviceAddress = vkGetBufferDeviceAddress(_card->device(),&bufferDeviceAddressInfo);
        }

        VulkanResourceDescriptorHeap::~VulkanResourceDescriptorHeap()
        {
            if (_allocation)
            {
                vmaUnmapMemory(_card->allocator(),_allocation);
                vmaDestroyBuffer(_card->allocator(),_buffer,_allocation);
            }
        }

        VulkanResourceDescriptorHeap::VulkanResourceDescriptorHeap(VulkanResourceDescriptorHeap&& from) noexcept
        {
            move(from);
        }

        VulkanResourceDescriptorHeap& VulkanResourceDescriptorHeap::operator=(VulkanResourceDescriptorHeap&& from) noexcept
        {
            move(from);
            return *this;
        }

        GraphicsCard* VulkanResourceDescriptorHeap::graphicsCard()
        {
            return _card;
        }

        uint32_t VulkanResourceDescriptorHeap::descriptorCount()
        {
            return _descriptorCount;
        }

        void VulkanResourceDescriptorHeap::setUniformTexture(uint32_t index, Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
        {
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            };


            auto descriptorInfo = vulkanTexture->descriptorInfo();
            descriptorInfo.subresourceRange.baseMipLevel = baseMip;
            descriptorInfo.subresourceRange.levelCount = mipCount;
            descriptorInfo.subresourceRange.baseArrayLayer = baseLayer;
            descriptorInfo.subresourceRange.layerCount = layerCount;
            VkImageDescriptorInfoEXT imageInfo
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .pView = &descriptorInfo,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
            };
            resourceDescriptorInfo.data.pImage = &imageInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        void VulkanResourceDescriptorHeap::setUnorderedAccessTexture(uint32_t index, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)
        {
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            };
            auto descriptorInfo = vulkanTexture->descriptorInfo();
            descriptorInfo.subresourceRange.baseMipLevel = mip;
            descriptorInfo.subresourceRange.levelCount = 1;
            descriptorInfo.subresourceRange.baseArrayLayer = baseLayer;
            descriptorInfo.subresourceRange.layerCount = layerCount;
            VkImageDescriptorInfoEXT imageInfo
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .pView = &descriptorInfo,
                .layout = VK_IMAGE_LAYOUT_GENERAL,
            };
            resourceDescriptorInfo.data.pImage = &imageInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        void VulkanResourceDescriptorHeap::setUniformStructuredBuffer(uint32_t index, Buffer* buffer, uint32_t offset, uint32_t length)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            };
            VkDeviceAddressRangeEXT rangeInfo
            {
                .address = vulkanBuffer->deviceAddress() + offset,
                .size = length,
            };
            resourceDescriptorInfo.data.pAddressRange = &rangeInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        void VulkanResourceDescriptorHeap::setStorageStructuredBuffer(uint32_t index, Buffer* buffer, uint64_t elementIndex,
                                                            uint64_t elementCount, uint64_t elementStride)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            };
            VkDeviceAddressRangeEXT rangeInfo
            {
                .address = vulkanBuffer->deviceAddress() + (elementIndex * elementStride),
                .size = elementCount*elementStride,
            };
            resourceDescriptorInfo.data.pAddressRange = &rangeInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        void VulkanResourceDescriptorHeap::setUniformTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format,
                                                                 uint64_t offset, uint64_t length)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            };
            VkTexelBufferDescriptorInfoEXT texelBufferInfo
            {
                .sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .format = VulkanBackend::nativeFormat(format).format,
                .addressRange =
                {
                    .address = vulkanBuffer->deviceAddress() + offset,
                    .size = length,
                }
            };

            resourceDescriptorInfo.data.pTexelBuffer = &texelBufferInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        void VulkanResourceDescriptorHeap::setStorageTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format,
                                                                 uint64_t offset, uint64_t length)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + (index * _descriptorSize), .size = _descriptorSize};

            VkResourceDescriptorInfoEXT resourceDescriptorInfo{
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            };
            VkTexelBufferDescriptorInfoEXT texelBufferInfo
            {
                .sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .format = VulkanBackend::nativeFormat(format).format,
                .addressRange =
                {
                    .address = vulkanBuffer->deviceAddress() + offset,
                    .size = length,
                }
            };

            resourceDescriptorInfo.data.pTexelBuffer = &texelBufferInfo;

            _card->vkWriteResourceDescriptors(_card->device(),1,&resourceDescriptorInfo,&hostAddressRange);
        }

        VkDeviceAddress VulkanResourceDescriptorHeap::deviceAddress() const
        {
            return _deviceAddress;
        }

        uint64_t VulkanResourceDescriptorHeap::size() const
        {
            return _descriptorCount * _descriptorSize;
        }

        uint64_t VulkanResourceDescriptorHeap::reserved() const
        {
            return _reserved;
        }

        void VulkanResourceDescriptorHeap::move(VulkanResourceDescriptorHeap& from)
        {
            _card = from._card;
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            _data = from._data;
            _deviceAddress = from._deviceAddress;
            _descriptorCount = from._descriptorCount;
            _descriptorSize = from._descriptorSize;
        }
    } // vulkan
} // slag