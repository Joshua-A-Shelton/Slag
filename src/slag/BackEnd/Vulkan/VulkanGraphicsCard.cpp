#define VMA_IMPLEMENTATION
#include "VulkanGraphicsCard.h"
#include "VulkanLib.h"
#include "VulkanCommandBuffer.h"

namespace slag
{
    namespace vulkan
    {
        VulkanGraphicsCard::VulkanGraphicsCard(vkb::Device& device)
        {
            this->_device = device.device;
            this->_physicalDevice = device.physical_device;
            _graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
            _transferQueue = device.get_queue(vkb::QueueType::transfer).value();
            _computeQueue = device.get_queue(vkb::QueueType::compute).value();
            _graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();
            _transferQueueFamily = device.get_queue_index(vkb::QueueType::transfer).value();
            _computeQueueFamily = device.get_queue_index(vkb::QueueType::compute).value();
            _properties = device.physical_device.properties;

            VmaAllocatorCreateInfo allocatorInfo = {};
            allocatorInfo.physicalDevice = _physicalDevice;
            allocatorInfo.device = _device;
            allocatorInfo.instance = VulkanLib::instance();
            vmaCreateAllocator(&allocatorInfo, &_allocator);
        }

        VulkanGraphicsCard::VulkanGraphicsCard(VulkanGraphicsCard&& from)
        {
            move(std::move(from));
        }

        VulkanGraphicsCard& VulkanGraphicsCard::operator=(VulkanGraphicsCard&& from)
        {
            move(std::move(from));
            return *this;
        }

        VulkanGraphicsCard::~VulkanGraphicsCard()
        {
            if(_device)
            {
                vmaDestroyAllocator(_allocator);
                vkDeviceWaitIdle(_device);
                vkDestroyDevice(_device, nullptr);
            }
        }

        void VulkanGraphicsCard::move(VulkanGraphicsCard&& from)
        {
            std::swap(_physicalDevice,from._physicalDevice);
            std::swap(_device,from._device);
            std::swap(_graphicsQueue,from._graphicsQueue);
            std::swap(_transferQueue, from._transferQueue);
            std::swap(_computeQueue, from._computeQueue);
            std::swap(_graphicsQueueFamily,from._graphicsQueueFamily);
            std::swap(_transferQueueFamily, from._transferQueueFamily);
            std::swap(_computeQueue,from._computeQueue);
            std::swap(_allocator,from._allocator);
            std::swap(_properties,from._properties);
        }

        VulkanGraphicsCard::operator VkPhysicalDevice() const
        {
            return _physicalDevice;
        }

        VulkanGraphicsCard::operator VkDevice() const
        {
            return _device;
        }

        VkPhysicalDevice VulkanGraphicsCard::physicalDevice()
        {
            return _physicalDevice;
        }

        VkDevice VulkanGraphicsCard::device()
        {
            return _device;
        }

        VmaAllocator VulkanGraphicsCard::memoryAllocator()
        {
            return _allocator;
        }

        VkQueue VulkanGraphicsCard::graphicsQueue()
        {
            return _graphicsQueue;
        }

        VkQueue VulkanGraphicsCard::transferQueue()
        {
            return _transferQueue;
        }

        VkQueue VulkanGraphicsCard::computeQueue()
        {
            return _computeQueue;
        }

        uint32_t VulkanGraphicsCard::graphicsQueueFamily()
        {
            return _graphicsQueueFamily;
        }

        uint32_t VulkanGraphicsCard::transferQueueFamily()
        {
            return _transferQueueFamily;
        }

        uint32_t VulkanGraphicsCard::computeQueueFamily()
        {
            return _transferQueueFamily;
        }

        const VkPhysicalDeviceProperties& VulkanGraphicsCard::properties()
        {
            return _properties;
        }

        void VulkanGraphicsCard::runOneTimeCommands(VkQueue submissionQueue, uint32_t queueFamily,std::function<void(VkCommandBuffer)> commands)
        {

            VkCommandPool pool;
            VkCommandPoolCreateInfo commandPoolInfo{};
            commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolInfo.pNext = nullptr;

            //the command pool will be one that can submit graphics commands
            commandPoolInfo.queueFamilyIndex = queueFamily;
            //we also want the pool to allow for resetting of individual command buffers
            commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

            if(vkCreateCommandPool(VulkanLib::graphicsCard()->device(), &commandPoolInfo, nullptr, &pool)!=VK_SUCCESS)
            {
                throw std::runtime_error("Unable to initialize local Command Pool");
            }

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = pool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer,&beginInfo);

            commands(commandBuffer);

            vkEndCommandBuffer(commandBuffer);


            VkFence finished;
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            vkCreateFence(VulkanLib::graphicsCard()->device(), &fenceInfo, nullptr,&finished);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(submissionQueue, 1, &submitInfo, finished);
            vkWaitForFences(_device,1,&finished,true, 1000000000);

            vkDestroyFence(_device,finished, nullptr);
            vkDestroyCommandPool(_device,pool, nullptr);

        }

        void VulkanGraphicsCard::executeArbitrary(std::function<void(CommandBuffer *)> execution, GraphicsCard::QueueType queue)
        {
            VkQueue submissionQueue = nullptr;
            uint32_t submissionQueueFamily = 0;
            switch (queue)
            {
                case QueueType::GRAPHICS:
                    submissionQueue = VulkanGraphicsCard::_graphicsQueue;
                    submissionQueueFamily = VulkanGraphicsCard::_graphicsQueueFamily;
                    break;
                case QueueType::TRANSFER:
                    submissionQueue = VulkanGraphicsCard::_transferQueue;
                    submissionQueueFamily = VulkanGraphicsCard::_transferQueueFamily;
                    break;
                case QueueType::COMPUTE:
                    submissionQueue = VulkanGraphicsCard::_computeQueue;
                    submissionQueueFamily = VulkanGraphicsCard::_computeQueueFamily;
                    break;
            }

            VulkanCommandBuffer cb(true,submissionQueue,submissionQueueFamily,true);
            cb.begin();

            execution(&cb);

            cb.end();

            VkFence finished;
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            vkCreateFence(VulkanLib::graphicsCard()->device(), &fenceInfo, nullptr,&finished);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &cb.vulkanCommandBuffer();

            vkQueueSubmit(submissionQueue, 1, &submitInfo, finished);
            vkWaitForFences(_device,1,&finished,true, 1000000000);

            vkDestroyFence(_device,finished, nullptr);
        }

    } // slag
} // vulkan