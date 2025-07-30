#include "VulkanCommandBuffer.h"

#include "VulkanGraphicsCard.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(GPUQueue::QueueType type)
        {
            _type = type;
            uint32_t family = 0;
            switch (_type)
            {
            case GPUQueue::QueueType::GRAPHICS:
                family = VulkanGraphicsCard::selected()->graphicsQueueFamily();
                break;
            case GPUQueue::QueueType::COMPUTE:
                family = VulkanGraphicsCard::selected()->computeQueueFamily();
                break;
            case GPUQueue::QueueType::TRANSFER:
                family = VulkanGraphicsCard::selected()->transferQueueFamily();
                break;
            }

            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            commandPoolInfo.queueFamilyIndex = family;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(VulkanGraphicsCard::selected()->device(), &commandPoolInfo, nullptr, &_pool)!=VK_SUCCESS)
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

            auto result = vkAllocateCommandBuffers(VulkanGraphicsCard::selected()->device(), &allocInfo, &_commandBuffer);
            SLAG_ASSERT(result == VK_SUCCESS && "failed to allocate command buffer!");
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if (_pool)
            {
                vkDestroyCommandPool(VulkanGraphicsCard::selected()->device(), _pool, nullptr);
            }
        }

        void VulkanCommandBuffer::bindDescriptorPool(DescriptorPool* pool)
        {
#ifdef SLAG_DEBUG
            _boundDescriptorPool = pool;
#endif
        }
    } // vulkan
} // slag
