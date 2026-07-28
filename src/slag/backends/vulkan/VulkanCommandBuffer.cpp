#include "VulkanCommandBuffer.h"

#include "VulkanGraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace vulkan
    {
        VulkanCommandBuffer::VulkanCommandBuffer(VulkanGraphicsCard* graphicsCard,QueueType type)
        {
            _graphicsCard = graphicsCard;
            _type = type;
            uint32_t family = 0;
            switch (_type)
            {
            case QueueType::GRAPHICS:
                family = _graphicsCard->graphicsFamilyIndex();
                break;
            case QueueType::COMPUTE:
                family = _graphicsCard->computeFamilyIndex();
                break;
            case QueueType::TRANSFER:
                family = _graphicsCard->transferFamilyIndex();
                break;
            }

            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            commandPoolInfo.queueFamilyIndex = family;
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(_graphicsCard->device(), &commandPoolInfo, nullptr, &_commandPool)!=VK_SUCCESS)
            {
                throw ResourceCreationError("Unable to create command buffer");
            }
            //command buffer
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;

            allocInfo.commandPool = _commandPool;

            allocInfo.commandBufferCount = 1;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            auto result = vkAllocateCommandBuffers(_graphicsCard->device(), &allocInfo, &_commandBuffer);
            if(result != VK_SUCCESS)
            {
                throw ResourceCreationError("Unable to allocate command buffer");
            }
        }

        VulkanCommandBuffer::~VulkanCommandBuffer()
        {
            if (_commandPool)
            {
                vkDestroyCommandPool(_graphicsCard->device(), _commandPool, nullptr);
            }
        }

        VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer&& from) noexcept
        {
            move(from);
        }

        VulkanCommandBuffer& VulkanCommandBuffer::operator=(VulkanCommandBuffer&& from) noexcept
        {
            move(from);
            return *this;
        }

        void VulkanCommandBuffer::move(VulkanCommandBuffer& from)
        {
            IVKCBMove(from);
        }
    } // vulkan
} // slag
