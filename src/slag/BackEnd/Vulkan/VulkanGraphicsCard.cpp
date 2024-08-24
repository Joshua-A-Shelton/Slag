#define VMA_IMPLEMENTATION
#include "VulkanGraphicsCard.h"
#include "VulkanLib.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(VkInstance instance,vkb::Device& device)
        {
            this->_device = device.device;
            this->_physicalDevice = device.physical_device;

            _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
            _transferQueueFamily = device.get_queue_index(vkb::QueueType::transfer).value();
            _computeQueueFamily = device.get_queue_index(vkb::QueueType::compute).value();

            VkQueue graphics = nullptr;
            vkGetDeviceQueue(_device,_graphicsQueueFamily,0,&graphics);
            if(graphics)
            {
                _graphicsQueue = new VulkanQueue(graphics, slag::GpuQueue::QueueType::Graphics);
            }

            VkQueue transfer = nullptr;
            vkGetDeviceQueue(_device,_graphicsQueueFamily,0,&transfer);
            if(transfer)
            {
                _graphicsQueue = new VulkanQueue(transfer, slag::GpuQueue::QueueType::Transfer);
            }

            VkQueue compute = nullptr;
            vkGetDeviceQueue(_device,_graphicsQueueFamily,0,&compute);
            if(compute)
            {
                _graphicsQueue = new VulkanQueue(compute, slag::GpuQueue::QueueType::Compute);
            }

            _properties = device.physical_device.properties;

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = instance;
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
                if(_graphicsQueue)
                {
                    delete _graphicsQueue;
                }
                if(_transferQueue)
                {
                    delete _transferQueue;
                }
                if(_computeQueue)
                {
                    delete _computeQueue;
                }

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
            std::swap(_transferQueue, from._transferQueue);
            std::swap(_computeQueue, from._computeQueue);
            std::swap(_graphicsQueueFamily,from._graphicsQueueFamily);
            std::swap(_transferQueueFamily, from._transferQueueFamily);
            std::swap(_computeQueue,from._computeQueue);
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


        uint32_t VulkanGraphicsCard::graphicsQueueFamily()
        {
            return _graphicsQueueFamily;
        }

        uint32_t VulkanGraphicsCard::transferQueueFamily()
        {
            return _transferQueueFamily;
        }

        uint32_t VulkanGraphicsCard::computeQueueFamily()
        {
            return _computeQueueFamily;
        }

        const VkPhysicalDeviceProperties& VulkanGraphicsCard::properties()
        {
            return _properties;
        }

        GpuQueue* VulkanGraphicsCard::GraphicsQueue()
        {
            return _graphicsQueue;
        }

        GpuQueue* VulkanGraphicsCard::TransferQueue()
        {
            return _transferQueue;
        }

        GpuQueue* VulkanGraphicsCard::ComputeQueue()
        {
            return _computeQueue;
        }

    } // vulkan
} // slag