#include "VulkanSamplerDescriptorHeap.h"

#include "VulkanGraphicsCard.h"
#include "VulkanSampler.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanSamplerDescriptorHeap::VulkanSamplerDescriptorHeap(VulkanGraphicsCard* card, uint32_t descriptorCount)
        {
            _card = card;
            auto heapDetails = _card->descriptorHeapDetails();
            _descriptorCount = descriptorCount;
            _descriptorSize = heapDetails.samplerDescriptorSize;
            SLAG_ASSERT(descriptorCount > 0 && "Descriptor count must be greater than 0");
            SLAG_ASSERT(descriptorCount <= heapDetails.maxSamplerDescriptors && "Exceeded max heap size");

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            allocationCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = (descriptorCount * heapDetails.samplerDescriptorSize) + heapDetails.samplerReservedRangeSize;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            _reserved = heapDetails.samplerReservedRangeSize;

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

        VulkanSamplerDescriptorHeap::~VulkanSamplerDescriptorHeap()
        {
            if (_allocation)
            {
                vmaUnmapMemory(_card->allocator(),_allocation);
                vmaDestroyBuffer(_card->allocator(),_buffer,_allocation);
            }
        }

        VulkanSamplerDescriptorHeap::VulkanSamplerDescriptorHeap(VulkanSamplerDescriptorHeap&& from) noexcept
        {
            move(from);
        }

        VulkanSamplerDescriptorHeap& VulkanSamplerDescriptorHeap::operator=(VulkanSamplerDescriptorHeap&& from) noexcept
        {
            move(from);
            return *this;
        }

        GraphicsCard* VulkanSamplerDescriptorHeap::graphicsCard()
        {
            return _card;
        }

        uint64_t VulkanSamplerDescriptorHeap::size()
        {
            return _descriptorCount * _descriptorSize;
        }

        void* VulkanSamplerDescriptorHeap::data()
        {
            return _data;
        }

        uint64_t VulkanSamplerDescriptorHeap::deviceAddress()
        {
            return _deviceAddress;
        }

        VkDeviceAddress VulkanSamplerDescriptorHeap::deviceAddress() const
        {
            return _deviceAddress;
        }

        uint64_t VulkanSamplerDescriptorHeap::reserved() const
        {
            return _reserved;
        }

        void VulkanSamplerDescriptorHeap::move(VulkanSamplerDescriptorHeap& from)
        {
            _card = from._card;
            std::swap(_buffer,from._buffer);
            std::swap(_allocation,from._allocation);
            _data = from._data;
            _reserved = from._reserved;
            _deviceAddress = from._deviceAddress;
            _descriptorCount = from._descriptorCount;
            _descriptorSize = from._descriptorSize;
        }
    } // vulkan
} // slag