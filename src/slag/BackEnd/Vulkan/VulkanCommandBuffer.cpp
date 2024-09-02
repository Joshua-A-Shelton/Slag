#include "VulkanCommandBuffer.h"
#include "VulkanLib.h"
#include "../../Resources/ResourceManager.h"
#include "VulkanTexture.h"

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
                    resources::ResourceManager::removeBufferFromActive(this);
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
            std::swap(_pool,from._pool);
            std::swap(_buffer,from._buffer);
            std::swap(_finished,from._finished);
            _family = from._family;
        }

        void VulkanCommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setBufferAsActive(this);
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
                resources::ResourceManager::removeBufferFromActive(this);
                freeResourceReferences();
                delete _finished;
                _finished = nullptr;
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

        void VulkanCommandBuffer::ClearColorImage(Texture* texture, ClearColor color,Texture::Layout layout)
        {
            auto tex = dynamic_cast<VulkanTexture*>(texture);
            VkImageSubresourceRange range{};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseArrayLayer = 0;
            range.baseMipLevel = 0;
            range.layerCount = 1;
            range.levelCount = tex->mipLevels();
            vkCmdClearColorImage(_buffer, tex->image(), VulkanLib::layout(layout),
                                 reinterpret_cast<const VkClearColorValue*>(&color), 1, &range);
        }
    } // vulkan
} // slag