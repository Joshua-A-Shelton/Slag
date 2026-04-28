#include "VulkanDescriptorHeap.h"

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSampler.h"
#include "VulkanTexture.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanDescriptorHeap::VulkanDescriptorHeap(VulkanGraphicsCard* graphicsCard, DescriptorHeapType type, uint32_t size)
        {
            _size = size;
            _graphicsCard = graphicsCard;
            _type = type;
            auto limits = _graphicsCard->descriptorHeapDetails();
            SLAG_ASSERT((type == DescriptorHeapType::RESOURCE && size <= limits.maxResourceDescriptorHeapSize) || (type == DescriptorHeapType::SAMPLER && size <= limits.maxSamplerDescriptorHeapSize) && "Exceeded max heap size");

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            //allocationCreateInfo.minAlignment = 32;

            _reserved = type == DescriptorHeapType::RESOURCE ? limits.resourceReservedRangeSize : limits.samplerReservedRangeSize;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = _size + _reserved;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            auto result = vmaCreateBufferWithAlignment(_graphicsCard->allocator(),&bufferCreateInfo,&allocationCreateInfo,32,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw ResourceCreationError("failed to create buffer");
            }
            vmaMapMemory(_graphicsCard->allocator(),_allocation,&_data);

            VkBufferDeviceAddressInfo bufferDeviceAddressInfo
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .buffer = _buffer,
            };
            _deviceAddress = vkGetBufferDeviceAddress(_graphicsCard->device(),&bufferDeviceAddressInfo);
        }

        VulkanDescriptorHeap::VulkanDescriptorHeap(VulkanDescriptorHeap&& from) noexcept
        {
            move(from);
        }

        VulkanDescriptorHeap& VulkanDescriptorHeap::operator=(VulkanDescriptorHeap&& from) noexcept
        {
            move(from);
            return *this;
        }

        VulkanDescriptorHeap::~VulkanDescriptorHeap()
        {
            if (_allocation)
            {
                vmaUnmapMemory(_graphicsCard->allocator(),_allocation);
                vmaDestroyBuffer(_graphicsCard->allocator(),_buffer,_allocation);
            }
        }

        void VulkanDescriptorHeap::writeSamplerDescriptor(uint32_t location, Sampler* sampler)
        {
            auto vulkanSampler = static_cast<VulkanSampler*>(sampler);
            auto vulkanHandle = vulkanSampler->vulkanHandle();
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().samplerDescriptorSize};
            _graphicsCard->vkWriteSamplerDescriptors(_graphicsCard->device(),1,&vulkanHandle,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeSampledTextureDescriptor(uint32_t location, Texture* texture)
        {
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().sampledTextureSize};
            auto& view = vulkanTexture->descriptorInfo();
            VkImageDescriptorInfoEXT imageDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
                .pView = &view,
                .layout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            };
            resourceDescriptorData.data.pImage = &imageDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeUnorderedAccessTextureDescriptor(uint32_t location, Texture* texture)
        {
            auto vulkanTexture = static_cast<VulkanTexture*>(texture);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().unorderedAccessTextureSize};
            auto& view = vulkanTexture->descriptorInfo();
            VkImageDescriptorInfoEXT imageDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
                .pView = &view,
                .layout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            };
            resourceDescriptorData.data.pImage = &imageDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeUniformBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset,
                                                                uint64_t length)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().uniformBufferSize};
            VkDeviceAddressRangeEXT  bufferDescriptorInfo
            {
                .address = vulkanBuffer->deviceAddress()+offset,
                .size = length
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            };
            resourceDescriptorData.data.pAddressRange = &bufferDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeUnorderedAccessBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().unorderedAccessBufferSize};
            VkDeviceAddressRangeEXT  bufferDescriptorInfo
            {
                .address = vulkanBuffer->deviceAddress()+offset,
                .size = length
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            };
            resourceDescriptorData.data.pAddressRange = &bufferDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeUniformTexelBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset,
                                                                     uint64_t length, PixelFormat format)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().uniformTexelBufferAlignment};
            VkTexelBufferDescriptorInfoEXT texelBufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .format = VulkanBackend::nativeFormat(format).format,
                .addressRange = {.address = vulkanBuffer->deviceAddress()+offset,.size = length}
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            };
            resourceDescriptorData.data.pTexelBuffer = &texelBufferDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        void VulkanDescriptorHeap::writeUnorderedAccessTexelBufferDescriptor(uint32_t location, Buffer* buffer,
                                                                             uint64_t offset, uint64_t length, PixelFormat format)
        {
            auto vulkanBuffer = static_cast<VulkanBuffer*>(buffer);
            VkHostAddressRangeEXT hostAddressRange{.address = ((unsigned char*)_data) + location, .size = _graphicsCard->descriptorTableDetails().unorderedAccessTexelBufferSize};
            VkTexelBufferDescriptorInfoEXT texelBufferDescriptorInfo
            {
                .sType = VK_STRUCTURE_TYPE_TEXEL_BUFFER_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .format = VulkanBackend::nativeFormat(format).format,
                .addressRange = {.address = vulkanBuffer->deviceAddress()+offset,.size = length}
            };
            VkResourceDescriptorInfoEXT resourceDescriptorData
            {
                .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
                .pNext = nullptr,
                .type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            };
            resourceDescriptorData.data.pTexelBuffer = &texelBufferDescriptorInfo;
            _graphicsCard->vkWriteResourceDescriptors(_graphicsCard->device(),1,&resourceDescriptorData,&hostAddressRange);
        }

        GraphicsCard* VulkanDescriptorHeap::graphicsCard()
        {
            return _graphicsCard;
        }

        uint32_t VulkanDescriptorHeap::size()
        {
            return _size;
        }

        uint32_t VulkanDescriptorHeap::reserved()
        {
            return _reserved;
        }

        DescriptorHeapType VulkanDescriptorHeap::type()
        {
            return _type;
        }

        VkDeviceAddress VulkanDescriptorHeap::deviceAddress() const
        {
            return _deviceAddress;
        }

        void VulkanDescriptorHeap::move(VulkanDescriptorHeap& from)
        {
            _size = from._size;
            _reserved = from._reserved;
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            std::swap(_data,from._data);
            _type = from._type;
            _deviceAddress = from._deviceAddress;
        }
    } // vulkan
} // slag