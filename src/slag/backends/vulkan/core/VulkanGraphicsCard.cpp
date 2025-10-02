#include "VulkanGraphicsCard.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSemaphore.h"
#include "VulkanTexture.h"
#include "../../Backend.h"
#include "slag/backends/vulkan/VulkanBackend.h"

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

            auto pqueue = device.get_dedicated_queue(vkb::QueueType::present);
            if(pqueue.has_value())
            {
                _presentQueue = pqueue.value();
            }
            else
            {
                pqueue = device.get_queue(vkb::QueueType::present);
                if (pqueue.has_value())
                {
                    _presentQueue = pqueue.value();
                }
                else
                {
                    _presentQueue = _graphicsQueue->vulkanHandle();
                    _seperatePresent = false;
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

        uint64_t VulkanGraphicsCard::uniformBufferOffsetAlignment()
        {
            VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            vkGetPhysicalDeviceProperties2(_physicalDevice,&properties);
            return properties.properties.limits.minUniformBufferOffsetAlignment;
        }

        uint64_t VulkanGraphicsCard::storageBufferOffsetAlignment()
        {
            VkPhysicalDeviceProperties2 properties{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            vkGetPhysicalDeviceProperties2(_physicalDevice,&properties);
            return properties.properties.limits.minStorageBufferOffsetAlignment;
        }

        void VulkanGraphicsCard::defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal,size_t signalCount)
        {
            if (waitFor!= nullptr && waitForCount > 0)
            {
                Semaphore::waitFor(waitFor, waitForCount);
            }

            VmaDefragmentationInfo defragInfo = {};
            defragInfo.flags = VMA_DEFRAGMENTATION_FLAG_ALGORITHM_FAST_BIT;


            VmaDefragmentationContext defragCtx;
            VkResult res = vmaBeginDefragmentation(_allocator, &defragInfo, &defragCtx);
            if (res != VK_SUCCESS)
            {
                throw std::runtime_error("failed to begin defragmentation");
            }
            while (true)
            {
                VmaDefragmentationPassMoveInfo pass;
                res = vmaBeginDefragmentationPass(_allocator, defragCtx, &pass);
                if(res == VK_SUCCESS)
                    break;
                else if(res == VK_INCOMPLETE)
                {
                    std::vector<VulkanBufferMoveData> movedBuffers;
                    std::vector<VulkanImageMoveData> movedTextures;
                    VulkanCommandBuffer transitionCB(GPUQueue::QueueType::GRAPHICS);
                    VulkanCommandBuffer moveCB(GPUQueue::QueueType::GRAPHICS);
                    VulkanSemaphore transitioned(0);
                    VulkanSemaphore moved(0);
                    transitionCB.begin();
                    moveCB.begin();
                    for(uint32_t i = 0; i < pass.moveCount; ++i)
                    {
                        // Inspect pass.pMoves[i].srcAllocation, identify what buffer/image it represents.
                        VmaAllocationInfo allocInfo;
                        vmaGetAllocationInfo(_allocator, pass.pMoves[i].srcAllocation, &allocInfo);
                        VulkanGPUMemoryReference* userData = (VulkanGPUMemoryReference*)allocInfo.pUserData;
                        if (userData->memoryType == VulkanGPUMemoryReference::MemoryType::TEXTURE)
                        {
                            auto texture = userData->reference.texture;
                            auto movedTexture = texture->moveMemory(pass.pMoves[i].dstTmpAllocation,&transitionCB,&moveCB);
                            if (movedTexture.movedSucceded)
                            {
                                movedTextures.push_back(movedTexture);
                            }
                            else
                            {
                                pass.pMoves[i].operation = VMA_DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                            }
                        }
                        else
                        {
                            auto buffer = userData->reference.buffer;
                            auto movedBuffer = buffer->moveMemory(pass.pMoves[i].dstTmpAllocation,&moveCB);
                            if (movedBuffer.movedSucceded)
                            {
                                movedBuffers.push_back(movedBuffer);
                            }
                            else
                            {
                                pass.pMoves[i].operation = VMA_DEFRAGMENTATION_MOVE_OPERATION_IGNORE;
                            }
                        }

                    }
                    transitionCB.end();
                    moveCB.end();
                    CommandBuffer* transitionedPtr = &transitionCB;
                    CommandBuffer* movedPtr = &moveCB;
                    SemaphoreValue transitionedValue{.semaphore = &transitioned,.value = 1};
                    SemaphoreValue movedValue{.semaphore = &moved,.value = 1};
                    QueueSubmissionBatch batches[]
                    {
                        {
                            .waitSemaphores = nullptr,
                            .waitSemaphoreCount = 0,
                            .commandBuffers = &transitionedPtr,
                            .commandBufferCount = 1,
                            .signalSemaphores = &transitionedValue,
                            .signalSemaphoreCount = 1,
                        },
                        {
                            .waitSemaphores = &transitionedValue,
                            .waitSemaphoreCount = 1,
                            .commandBuffers = &movedPtr,
                            .commandBufferCount = 1,
                            .signalSemaphores = &movedValue,
                            .signalSemaphoreCount = 1,
                        }
                    };

                    _graphicsQueue->submit(batches,2);
                    moved.waitForValue(1);
                    for (auto i=0; i< movedTextures.size(); i++)
                    {
                        vkDestroyImage(_device,movedTextures[i].image,nullptr);
                        vkDestroyImageView(_device,movedTextures[i].view,nullptr);
                    }
                    movedTextures.clear();
                    for (auto i=0; i< movedBuffers.size(); i++)
                    {
                        vkDestroyBuffer(_device,movedBuffers[i].buffer,nullptr);
                    }
                    movedBuffers.clear();
                    res = vmaEndDefragmentationPass(_allocator, defragCtx, &pass);
                    if(res == VK_SUCCESS)
                        break;
                    else if(res != VK_INCOMPLETE)
                    {
                        throw std::runtime_error("failed to defragment graphics memory");
                    }

                }
                else
                {
                    throw std::runtime_error("failed to defragment graphics memory");
                }
            }
            vmaEndDefragmentation(_allocator,defragCtx,nullptr);

            if (signal != nullptr && signalCount > 0)
            {
                for (auto i=0; i<signalCount; i++)
                {
                    auto current = signal[i];
                    current.semaphore->signal(current.value);
                }
            }
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

        VkQueue VulkanGraphicsCard::presentQueue() const
        {
            return _presentQueue;
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
            _seperatePresent = from._seperatePresent;
            _seperateCompute=from._seperateCompute;
            _seperateTransfer=from._seperateTransfer;
        }
    } // vulkan
} // slag
