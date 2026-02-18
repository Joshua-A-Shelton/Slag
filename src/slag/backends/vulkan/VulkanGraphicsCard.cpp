#include "VulkanGraphicsCard.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanSubmissionQueue.h"
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

            //guaranteed to have this, everything will fall back to this if they don't have one
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
                _graphicsQueue = new VulkanSubmissionQueue(this,gqueue.value(), QueueType::GRAPHICS);
            }
            else
            {
                gqueue = device.get_queue(vkb::QueueType::graphics);
                _graphicsQueue = new VulkanSubmissionQueue(this,gqueue.value(),QueueType::GRAPHICS);
            }

            auto cqueue = device.get_dedicated_queue(vkb::QueueType::compute);
            if(cqueue.has_value())
            {
                _computeQueue = new VulkanSubmissionQueue(this,cqueue.value(), QueueType::COMPUTE);
            }
            else
            {
                cqueue = device.get_queue(vkb::QueueType::compute);
                if(cqueue.has_value())
                {
                    _computeQueue = new VulkanSubmissionQueue(this,cqueue.value(), QueueType::COMPUTE);
                }
                else
                {
                    _computeQueue = _graphicsQueue;
                }
            }

            auto tqueue = device.get_dedicated_queue(vkb::QueueType::transfer);
            if(tqueue.has_value())
            {
                _transferQueue = new VulkanSubmissionQueue(this,tqueue.value(), QueueType::TRANSFER);
            }
            else
            {
                tqueue = device.get_queue(vkb::QueueType::transfer);
                if(tqueue.has_value())
                {
                    _transferQueue = new VulkanSubmissionQueue(this,tqueue.value(), QueueType::TRANSFER);
                }
                else
                {
                    _transferQueue = _computeQueue;
                }
            }


            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = instance;
            allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
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
                if (!(_transferQueue == _computeQueue || _transferQueue == _graphicsQueue))
                {
                    delete _transferQueue;
                }
                if (_computeQueue != _graphicsQueue)
                {
                    delete _computeQueue;
                }
                delete _graphicsQueue;
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

        uint64_t VulkanGraphicsCard::maxShaderAccessReadOnlyBufferSize() const
        {
            return _physicalDeviceProperties.limits.maxUniformBufferRange;
        }

        bool VulkanGraphicsCard::cacheCoherentSharedMemory() const
        {
            return _cacheCoherentSharedMemory;
        }

        SubmissionQueue* VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        SubmissionQueue* VulkanGraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        SubmissionQueue* VulkanGraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        uint64_t VulkanGraphicsCard::defragmentMemory(SemaphoreValue* waitFor, uint32_t waitCount,
                                                      SemaphoreValue* signal, uint32_t signalCount, uint64_t targetBytes)
        {
            throw NotImplemented();
        }

        CommandBuffer* VulkanGraphicsCard::newCommandBuffer(QueueType type)
        {
            return new VulkanCommandBuffer(this, type,CommandBufferLevel::PRIMARY);
        }

        Semaphore* VulkanGraphicsCard::newSemaphore(uint64_t initialValue)
        {
           return new VulkanSemaphore(this,initialValue);
        }

        Buffer* VulkanGraphicsCard::newBuffer(
            uint64_t size,
            BufferUsage usage,
            BufferShaderAccess shaderAccess,
            BufferCPUAccess cpuAccess)
        {
            return new VulkanBuffer(this, size, usage, shaderAccess, cpuAccess);
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard& from)
        {
            _name.swap(from._name);
            std::swap(_physicalDevice,from._physicalDevice);
            std::swap(_device,from._device);
            _physicalDeviceProperties=from._physicalDeviceProperties;
            _physicalDeviceMemoryProperties=from._physicalDeviceMemoryProperties;
            std::swap(_allocator,from._allocator);
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_computeQueue,from._computeQueue);
            std::swap(_transferQueue,from._transferQueue);
            _videoMemory = from._videoMemory;
            _graphicsQueueFamily=from._graphicsQueueFamily;
            _computeQueueFamily=from._computeQueueFamily;
            _transferQueueFamily=from._transferQueueFamily;
            _cacheCoherentSharedMemory = from._cacheCoherentSharedMemory;
        }

        VkDevice VulkanGraphicsCard::device() const
        {
            return _device;
        }

        VmaAllocator VulkanGraphicsCard::allocator() const
        {
            return _allocator;
        }

        uint32_t VulkanGraphicsCard::graphicsFamilyIndex() const
        {
            return _graphicsQueueFamily;
        }

        uint32_t VulkanGraphicsCard::computeFamilyIndex() const
        {
            return _computeQueueFamily;
        }

        uint32_t VulkanGraphicsCard::transferFamilyIndex() const
        {
            return _transferQueueFamily;
        }
    } // vulkan
} // slag
