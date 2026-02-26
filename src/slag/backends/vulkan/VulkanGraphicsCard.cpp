#include "VulkanGraphicsCard.h"

#include "VulkanBackend.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanSubmissionQueue.h"
#include "VulkanTexture.h"
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

        uint64_t VulkanGraphicsCard::maxShaderAccessUniformBufferSize() const
        {
            return _physicalDeviceProperties.limits.maxUniformBufferRange;
        }

        PixelFormatProperties VulkanGraphicsCard::formatProperties(PixelFormat format) const
        {
            auto nativeFormat = VulkanBackend::nativeFormat(format);
            PixelFormatProperties properties{};

            VkFormatProperties2 formatProperties{.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
            vkGetPhysicalDeviceFormatProperties2(_physicalDevice,nativeFormat.format,&formatProperties);

            VkFormatFeatureFlags2 features = 0;
            TextureUsageFlags usage =static_cast<TextureUsageFlags>(0);

            if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = TextureTiling::OPTIMIZED;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else if (formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT && formatProperties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)
            {
                properties.tiling = TextureTiling::LINEAR;
                features = formatProperties.formatProperties.optimalTilingFeatures;
            }
            else
            {
                properties.tiling = TextureTiling::UNSUPPORTED;
                return properties;
            }
            bool hasUsage = false;
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)
            {
                usage |= TextureUsageFlags::SAMPLED;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT )
            {
                usage |= TextureUsageFlags::UNORDERED_ACCESS;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)
            {
                usage |= TextureUsageFlags::COLOR_TARGET;
                hasUsage = true;
            }
            if (features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                usage |= TextureUsageFlags::DEPTH_STENCIL_TARGET;
                hasUsage = true;
            }
            properties.validUsageFlags = usage;
            if (!hasUsage)
            {
                properties.tiling = TextureTiling::UNSUPPORTED;
                return properties;
            }
            if (features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
            {
                properties.linearFilteringCapable = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_SRC_BIT )
            {
                properties.blitSource = true;
            }
            if (features & VK_FORMAT_FEATURE_2_BLIT_DST_BIT )
            {
                properties.blitDestination = true;
            }

            return properties;
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
            BufferMemoryType memoryType,
            BufferCPUAccess cpuAccess)
        {
            return new VulkanBuffer(this, size, memoryType, cpuAccess);
        }

        Texture* VulkanGraphicsCard::newTexture(uint32_t width, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels, uint32_t layers)
        {
            return new VulkanTexture(this,width,format,usage,mipLevels,layers);
        }

        Texture* VulkanGraphicsCard::newTexture(uint32_t width, uint32_t height, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,
            SampleCount sampleCount, uint32_t layers)
        {
            return new VulkanTexture(this,width,height,format,usage,mipLevels,sampleCount,layers);
        }

        Texture* VulkanGraphicsCard::newTexture(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, TextureUsageFlags usage,
            uint32_t mipLevels)
        {
            return new VulkanTexture(this,width,height,depth,format,usage,mipLevels);
        }

        Texture* VulkanGraphicsCard::newTextureCube(uint32_t dimension, PixelFormat format, TextureUsageFlags usage, uint32_t mipLevels,
            uint32_t arrayDepth)
        {
            return new VulkanTexture(this,format,usage,dimension,mipLevels,arrayDepth);
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
