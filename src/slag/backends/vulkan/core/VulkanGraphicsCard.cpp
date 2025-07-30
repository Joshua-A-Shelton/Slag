#include "VulkanGraphicsCard.h"

#include "VulkanCommandBuffer.h"
#include "VulkanTexture.h"
#include "../../Backend.h"
namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard* VulkanGraphicsCard::selected()
        {
            return static_cast<VulkanGraphicsCard*>(slagGraphicsCard());
        }

        VulkanGraphicsCard::VulkanGraphicsCard(VkInstance instance, const vkb::Device& device)
        {
            _device = device.device;
            _physicalDevice = device.physical_device;

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
                _graphicsQueue = new VulkanQueue(gqueue.value(), GPUQueue::QueueType::GRAPHICS);
            }
            else
            {
                gqueue = device.get_queue(vkb::QueueType::graphics);
                _graphicsQueue = new VulkanQueue(gqueue.value(),GPUQueue::QueueType::GRAPHICS);
            }

            auto cqueue = device.get_dedicated_queue(vkb::QueueType::compute);
            if(cqueue.has_value())
            {
                _computeQueue = new VulkanQueue(cqueue.value(), GPUQueue::QueueType::COMPUTE);
            }
            else
            {
                cqueue = device.get_queue(vkb::QueueType::compute);
                if(cqueue.has_value())
                {
                    _computeQueue = new VulkanQueue(cqueue.value(), GPUQueue::QueueType::COMPUTE);
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
                _transferQueue = new VulkanQueue(tqueue.value(), GPUQueue::QueueType::TRANSFER);
            }
            else
            {
                tqueue = device.get_queue(vkb::QueueType::transfer);
                if(tqueue.has_value())
                {
                    _transferQueue = new VulkanQueue(tqueue.value(), GPUQueue::QueueType::TRANSFER);
                }
                else
                {
                    _transferQueue = _computeQueue;
                    _seperateTransfer = false;
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
            move(from);
        }

        VulkanGraphicsCard& VulkanGraphicsCard::operator=(VulkanGraphicsCard&& from)
        {
            move(from);
            return *this;
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

        uint64_t VulkanGraphicsCard::videoMemory()
        {
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memoryProperties);
            uint64_t memorySize = 0;
            for (auto i=0; i<memoryProperties.memoryHeapCount; i++)
            {
                auto& heap = memoryProperties.memoryHeaps[i];
                if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    memorySize += heap.size;
                }
            }
            return memorySize;
        }

        std::string VulkanGraphicsCard::name()
        {
            return _properties.deviceName;
        }

        GPUQueue* VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        GPUQueue* VulkanGraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        GPUQueue* VulkanGraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        void VulkanGraphicsCard::defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal,size_t signalCount)
        {
            //see https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/defragmentation.html
            //https://www.khronos.org/blog/copying-images-on-the-host-in-vulkan
            //https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_host_image_copy.html
            throw std::runtime_error("not implemented");
        }

        VkPhysicalDevice VulkanGraphicsCard::physicalDevice() const
        {
            return _physicalDevice;
        }

        VkDevice VulkanGraphicsCard::device() const
        {
            return _device;
        }

        VmaAllocator VulkanGraphicsCard::allocator() const
        {
            return _allocator;
        }

        uint32_t VulkanGraphicsCard::graphicsQueueFamily() const
        {
            return _graphicsQueueFamily;
        }

        uint32_t VulkanGraphicsCard::computeQueueFamily() const
        {
            return _computeQueueFamily;
        }

        uint32_t VulkanGraphicsCard::transferQueueFamily() const
        {
            return _transferQueueFamily;
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard& from)
        {
            std::swap(_physicalDevice,from._physicalDevice);
            std::swap(_device,from._device);
            std::swap(_allocator, from._allocator);
            std::swap(_properties, from._properties);
            _graphicsQueueFamily=from._graphicsQueueFamily;
            _computeQueueFamily=from._computeQueueFamily;
            _transferQueueFamily=from._transferQueueFamily;
        }
    } // vulkan
} // slag
