#define VMA_IMPLEMENTATION
#include "VulkanGraphicsCard.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"
#include "VulkanGPUMemoryReference.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(VkInstance instance,vkb::Device& device)
        {
            this->_device = device.device;
            this->_physicalDevice = device.physical_device;

            _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
            auto tqf = device.get_queue_index(vkb::QueueType::transfer);
            if(tqf.has_value())
            {
                _transferQueueFamily = tqf.value();
            }
            else
            {
                _transferQueueFamily = _graphicsQueueFamily;
            }
            auto cqf = device.get_queue_index(vkb::QueueType::compute);
            if(cqf.has_value())
            {
                _computeQueueFamily = cqf.value();
            }
            else
            {
                _computeQueueFamily = _graphicsQueueFamily;
            }

            auto gqueue = device.get_dedicated_queue(vkb::QueueType::graphics);
            if(gqueue.has_value())
            {
                _graphicsQueue = new VulkanQueue(gqueue.value(), slag::GpuQueue::QueueType::GRAPHICS);
            }
            else
            {
                gqueue = device.get_queue(vkb::QueueType::graphics);
                _graphicsQueue = new VulkanQueue(gqueue.value(),slag::GpuQueue::QueueType::GRAPHICS);
            }

            auto cqueue = device.get_dedicated_queue(vkb::QueueType::compute);
            if(cqueue.has_value())
            {
                _computeQueue = new VulkanQueue(cqueue.value(), slag::GpuQueue::QueueType::COMPUTE);
            }
            else
            {
                cqueue = device.get_queue(vkb::QueueType::compute);
                if(cqueue.has_value())
                {
                    _computeQueue = new VulkanQueue(cqueue.value(), slag::GpuQueue::QueueType::COMPUTE);
                }
                else
                {
                    _computeQueue = _graphicsQueue;
                    _seperateCompute = false;
                }
            }

            auto tqueue = device.get_dedicated_queue(vkb::QueueType::transfer);
            if(tqueue.has_value())
            {
                _transferQueue = new VulkanQueue(tqueue.value(), slag::GpuQueue::QueueType::TRANSFER);
            }
            else
            {
                tqueue = device.get_queue(vkb::QueueType::transfer);
                if(tqueue.has_value())
                {
                    _transferQueue = new VulkanQueue(tqueue.value(), slag::GpuQueue::QueueType::TRANSFER);
                }
                else
                {
                    _transferQueue = _computeQueue;
                    _seperateTransfer = false;
                }
            }

            auto pqueue = device.get_dedicated_queue(vkb::QueueType::present);
            if(pqueue.has_value())
            {
                _presentQueue = pqueue.value();
            }
            else
            {
                pqueue = device.get_queue(vkb::QueueType::present);
                if(pqueue.has_value())
                {
                    _presentQueue = pqueue.value();
                }
                else
                {
                    _presentQueue = _transferQueue->underlyingQueue();
                }
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
                if(_transferQueue && _seperateTransfer)
                {
                    delete _transferQueue;
                }
                if(_computeQueue && _seperateCompute)
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

        GpuQueue* VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        GpuQueue* VulkanGraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        GpuQueue* VulkanGraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        VkQueue VulkanGraphicsCard::presentQueue()
        {
            return _presentQueue;
        }

        void VulkanGraphicsCard::defragmentMemory()
        {
            //see https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/defragmentation.html
            //https://www.khronos.org/blog/copying-images-on-the-host-in-vulkan
            //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_host_image_copy.html
            throw std::runtime_error("not implemented");

        }

    } // vulkan
} // slag