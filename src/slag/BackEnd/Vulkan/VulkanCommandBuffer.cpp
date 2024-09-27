#include "VulkanCommandBuffer.h"
#include "../../Resources/ResourceManager.h"
#include "VulkanLib.h"


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
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if(_pool)
            {
                //there's the possibility we started recording commands, but never submitted. Force clear resources just in case
                if(_finished == nullptr)
                {
                    resources::ResourceManager::removeConsumerFromActive(this);
                    freeResourceReferences();
                }
                else
                {
                    _waitUntilFinished();
                    delete _finished;
                }
                vkDestroyCommandPool(VulkanLib::card()->device(), _pool, nullptr);
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
            IVulkanCommandBuffer::move(from);
            ResourceConsumer::move(from);
            std::swap(_finished,from._finished);

        }

        void VulkanCommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setConsumerAsActive(this);
            VkCommandBufferBeginInfo cmdBeginInfo = {};
            cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmdBeginInfo.pNext = nullptr;

            cmdBeginInfo.pInheritanceInfo = nullptr;
            cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(_buffer,&cmdBeginInfo);
        }


        void VulkanCommandBuffer::waitUntilFinished()
        {
            _waitUntilFinished();
        }

        bool VulkanCommandBuffer::isFinished()
        {
            if(_finished)
            {
                return _finished->value()>=1;
            }
            return true;
        }

        void VulkanCommandBuffer::_waitUntilFinished()
        {
            if(_finished)
            {
                _finished->waitForValue(1);
                resources::ResourceManager::removeConsumerFromActive(this);
                freeResourceReferences();
                delete _finished;
                _finished = nullptr;
            }
        }

        void VulkanCommandBuffer::bindDescriptorPool(DescriptorPool* pool)
        {
            //Do nothing, vulkan doesn't require binding descriptor pools! :)
        }
    } // vulkan
} // slag