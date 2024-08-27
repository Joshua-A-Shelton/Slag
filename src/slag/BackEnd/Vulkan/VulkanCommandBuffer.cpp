#include "VulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "../../Resources/ResourceManager.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(uint32_t queueFamily)
        {
            _family = queueFamily;
            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            commandPoolInfo.queueFamilyIndex = queueFamily;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(VulkanLib::card()->device(), &commandPoolInfo, nullptr, &_pool)!=VK_SUCCESS)
            {
                throw std::runtime_error("Unable to initialize Command Pool");
            }
            //command buffer
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;

            allocInfo.commandPool = _pool;

            allocInfo.commandBufferCount = 1;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            auto result = vkAllocateCommandBuffers(VulkanLib::card()->device(), &allocInfo, &_buffer);
            assert(result == VK_SUCCESS && "failed to allocate command buffer!");
            _finished = new VulkanSemaphore(2, true);
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if(_pool)
            {
                //there's the possibility we started recording commands, but never submitted. Force clear resources just in case
                if(_finished->value() == 2)
                {
                    resources::ResourceManager::removeBufferFromActive(this);
                    freeResourceReferences();
                }
                else
                {
                    _waitUntilFinished();
                }
                vkDestroyCommandPool(VulkanLib::card()->device(), _pool, nullptr);
                delete _finished;
            }
        }

        VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& from)
        {
            move(std::move(from));
        }

        VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanCommandBuffer::move(VulkanCommandBuffer&& from)
        {
            std::swap(_pool,from._pool);
            std::swap(_buffer,from._buffer);
            std::swap(_finished,from._finished);
            _family = from._family;
        }

        void VulkanCommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setBufferAsActive(this);
            vkResetCommandBuffer(_buffer,0);
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(_buffer,&cmdBeginInfo);
        }

        void VulkanCommandBuffer::end()
        {
            vkEndCommandBuffer(_buffer);
        }

        void VulkanCommandBuffer::waitUntilFinished()
        {
            _waitUntilFinished();
        }

        bool VulkanCommandBuffer::isFinished()
        {
            return _finished->value()>=1;
        }

        void VulkanCommandBuffer::_waitUntilFinished()
        {
            if(_finished->value()==0)
            {
                _finished->waitForValue(1);
            }
            else if(_finished->value()==1)
            {
                resources::ResourceManager::removeBufferFromActive(this);
                freeResourceReferences();
                _finished->signal(2);
            }

        }

        GpuQueue::QueueType VulkanCommandBuffer::commandType()
        {
            if(_family == VulkanLib::card()->graphicsQueueFamily())
            {
                return GpuQueue::Graphics;
            }
            else if(_family == VulkanLib::card()->computeQueueFamily())
            {
                return GpuQueue::Compute;
            }
            else
            {
                return GpuQueue::Transfer;
            }
        }
    } // vulkan
} // slag