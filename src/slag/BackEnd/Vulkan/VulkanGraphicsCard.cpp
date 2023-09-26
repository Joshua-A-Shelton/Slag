#define VMA_IMPLEMENTATION
#include "VulkanGraphicsCard.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(vkb::Device& device)
        {
            this->_device = device.device;
            this->_physicalDevice = device.physical_device;
            _graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
            _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
            _properties = device.physical_device.properties;

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = VulkanLib::instance();
            vmaCreateAllocator(&allocatorInfo, &_allocator);
        }

        VulkanGraphicsCard::VulkanGraphicsCard(VulkanGraphicsCard&& from)
        {
            move(std::move(from));
        }

        VulkanGraphicsCard& VulkanGraphicsCard::operator=(VulkanGraphicsCard&& from)
        {
            move(std::move(from));
            return *this;
        }

        VulkanGraphicsCard::~VulkanGraphicsCard()
        {
            if(_device)
            {
                vmaDestroyAllocator(_allocator);
                vkDeviceWaitIdle(_device);
                vkDestroyDevice(_device, nullptr);
            }
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard&& from)
        {
            std::swap(_physicalDevice,from._physicalDevice);
            std::swap(_device,from._device);
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_graphicsQueueFamily,from._graphicsQueueFamily);
            std::swap(_allocator,from._allocator);
            std::swap(_properties,from._properties);
        }

        VulkanGraphicsCard::operator VkPhysicalDevice() const
        {
            return _physicalDevice;
        }

        VulkanGraphicsCard::operator VkDevice() const
        {
            return _device;
        }

        VkPhysicalDevice VulkanGraphicsCard::physicalDevice()
        {
            return _physicalDevice;
        }

        VkDevice VulkanGraphicsCard::device()
        {
            return _device;
        }

        VmaAllocator VulkanGraphicsCard::memoryAllocator()
        {
            return _allocator;
        }

        VkQueue VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        uint32_t VulkanGraphicsCard::graphicsQueueFamily()
        {
            return _graphicsQueueFamily;
        }

        const VkPhysicalDeviceProperties& VulkanGraphicsCard::properties()
        {
            return _properties;
        }
    } // slag
} // vulkan