#include "VulkanGraphicsCard.h"

#include "slag/exceptions/NotImplemented.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(VkInstance instance, const vkb::Device& device)
        {
            _device = device.device;
            _physicalDevice = device.physical_device.physical_device;
            _physicalDeviceProperties = device.physical_device.properties;
            _name = device.physical_device.name;
            _physicalDeviceMemoryProperties = device.physical_device.memory_properties;
            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = instance;
            vmaCreateAllocator(&allocatorInfo, &_allocator);

            for (int i=0; i< _physicalDeviceMemoryProperties.memoryHeapCount; i++)
            {
                auto heap = _physicalDeviceMemoryProperties.memoryHeaps[i];
                if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    _videoMemory += heap.size;
                }
            }
            _cacheCoherentSharedMemory = true;
            for (int i=0; i<_physicalDeviceMemoryProperties.memoryTypeCount; i++)
            {
                auto type = _physicalDeviceMemoryProperties.memoryTypes[i];
                if (type.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
                    if ((!(type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
                    {
                        _cacheCoherentSharedMemory = false;
                        break;
                    }
                }
            }
        }

        VulkanGraphicsCard::~VulkanGraphicsCard()
        {
            if (_device)
            {
                vmaDestroyAllocator(_allocator);
                vkDeviceWaitIdle(_device);
                vkDestroyDevice(_device, nullptr);
            }
        }

        VulkanGraphicsCard::VulkanGraphicsCard(VulkanGraphicsCard&& from)
        {
            move(from);
        }

        VulkanGraphicsCard& VulkanGraphicsCard::operator=(VulkanGraphicsCard&& from)
        {
            move(from);
            return *this;
        }

        std::string VulkanGraphicsCard::name() const
        {
            return _name;
        }

        uint64_t VulkanGraphicsCard::videoMemory() const
        {
            return _videoMemory;
        }

        bool VulkanGraphicsCard::cacheCoherentSharedMemory() const
        {
            return _cacheCoherentSharedMemory;
        }

        Buffer* VulkanGraphicsCard::newBuffer(
            uint64_t size,
            BufferUsage usage,
            BufferShaderAccess shaderAccess,
            BufferCPUAccess cpuAccess)
        {
            throw NotImplemented();
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard& from)
        {
            _name.swap(from._name);
            std::swap(_physicalDevice, from._physicalDevice);
            std::swap(_device, from._device);
            std::swap(_physicalDeviceProperties, from._physicalDeviceProperties);
            std::swap(_physicalDeviceMemoryProperties, from._physicalDeviceMemoryProperties);
            std::swap(_allocator, from._allocator);
        }
    } // vulkan
} // slag
