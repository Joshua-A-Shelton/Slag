#include "VulkanBuffer.h"

#include "VulkanCommandBuffer.h"
#include "VulkanGraphicsCard.h"
#include "VulkanSemaphore.h"
#include "slag/backends/vulkan/VulkanBackend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanBuffer::VulkanBuffer(size_t size, Accessibility accessibility, UsageFlags usage)
        {
            _size = size;
            _accessibility = accessibility;
            _usageFlags=usage;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            //buffer is created on the gpu
            allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            if (accessibility == Accessibility::CPU_AND_GPU)
            {
                //buffer is visible and consistent from the CPU
                allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
                allocationCreateInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
            allocationCreateInfo.pUserData = &_selfReference;

            VkBufferCreateInfo bufferCreateInfo{};
            bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCreateInfo.size = _size;
            bufferCreateInfo.usage = VulkanBackend::vulkanizedBufferUsage(usage);

            auto result = vmaCreateBuffer(VulkanGraphicsCard::selected()->allocator(),&bufferCreateInfo,&allocationCreateInfo,&_buffer,&_allocation,nullptr);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create buffer");
            }
            if (_accessibility == Accessibility::CPU_AND_GPU)
            {
                vmaMapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation,&_memoryLocation);
            }
        }

        VulkanBuffer::VulkanBuffer(void* data, size_t size, Accessibility accessibility, UsageFlags usage):VulkanBuffer(size,accessibility,usage)
        {
            VulkanBuffer::update(0,data,size,nullptr,0,nullptr,0);
        }

        VulkanBuffer::~VulkanBuffer()
        {
            if (_allocation)
            {
                if (_accessibility == Accessibility::CPU_AND_GPU)
                {
                    vmaUnmapMemory(VulkanGraphicsCard::selected()->allocator(),_allocation);
                }
                vmaDestroyBuffer(VulkanGraphicsCard::selected()->allocator(),_buffer,_allocation);
            }
        }

        VulkanBuffer::VulkanBuffer(VulkanBuffer&& from)
        {
            move(from);
        }

        VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& from)
        {
            move(from);
            return *this;
        }

        Buffer::Accessibility VulkanBuffer::accessibility()
        {
            return _accessibility;
        }

        uint64_t VulkanBuffer::size()
        {
            return _size;
        }

        Buffer::UsageFlags VulkanBuffer::usage()
        {
            return _usageFlags;
        }

        void VulkanBuffer::update(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait,size_t waitCount, SemaphoreValue* signal, size_t signalCount)
        {
            if (_accessibility == Accessibility::CPU_AND_GPU)
            {
                cpuUpdate(offset,data,dataLength,wait,waitCount,signal,signalCount);
            }
            else
            {
                gpuUpdate(offset,data,dataLength,wait,waitCount,signal,signalCount);
            }
        }

        void* VulkanBuffer::cpuHandle()
        {
            SLAG_ASSERT(_accessibility == Accessibility::CPU_AND_GPU && "Buffer must be CPU accesible");
            return _memoryLocation;
        }

        VkBuffer VulkanBuffer::vulkanHandle() const
        {
            return _buffer;
        }

        void VulkanBuffer::move(VulkanBuffer& from)
        {
            std::swap(_buffer, from._buffer);
            std::swap(_allocation,from._allocation);
            _accessibility = from._accessibility;
            _usageFlags = from._usageFlags;
            _size = from._size;
            std::swap(_memoryLocation, from._memoryLocation);
            if(_allocation)
            {
                vmaSetAllocationUserData(VulkanGraphicsCard::selected()->allocator(),_allocation,&_selfReference);
            }
        }

        void VulkanBuffer::cpuUpdate(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait,size_t waitCount, SemaphoreValue* signal, size_t signalCount)
        {
            SLAG_ASSERT(offset + dataLength <= _size && "Update exceeds size of buffer");
            if (waitCount)
            {
                SLAG_ASSERT(wait != nullptr);
                Semaphore::waitFor(wait,waitCount);
            }
            memcpy(_memoryLocation,data,dataLength);
            if (signalCount)
            {
                SLAG_ASSERT(signal != nullptr);
                for (size_t i=0; i< signalCount; i++)
                {
                    auto& semaphore = signal[i];
                    semaphore.semaphore->signal(semaphore.value);
                }
            }

        }

        void VulkanBuffer::gpuUpdate(uint64_t offset, void* data, uint64_t dataLength, SemaphoreValue* wait, size_t waitCount, SemaphoreValue* signal, size_t signalCount)
        {
            SLAG_ASSERT(offset + dataLength <= _size && "Update exceeds size of buffer");
            VulkanCommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            VulkanSemaphore finished(0);
            //this should technically be faster to not send that data initially, as we're not waiting on any semaphores this way
            VulkanBuffer buffer(dataLength,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            memcpy(buffer._memoryLocation,data,dataLength);

            commandBuffer.begin();
            commandBuffer.copyBufferToBuffer(&buffer,0,dataLength,this,offset);
            commandBuffer.end();

            CommandBuffer* ptr = &commandBuffer;
            std::vector<SemaphoreValue> signals(signalCount+1);
            for (int i=0; i< signalCount; i++)
            {
                signals[i] = signal[i];
            }
            signals[signalCount] = {.semaphore = &finished,.value = 1};
            VulkanGraphicsCard::selected()->transferQueue()->submit(&ptr,1,wait,waitCount,signals.data(),signals.size());
            finished.waitForValue(1);

        }
    } // vulkan
} // slag
