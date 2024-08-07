#ifndef SLAG_VULKANGRAPHICSCARD_H
#define SLAG_VULKANGRAPHICSCARD_H
#include "../../GraphicsCard.h"
#include <vulkan/vulkan.h>
#include <functional>
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard: public GraphicsCard
        {
        public:
            VulkanGraphicsCard(vkb::Device& device);
            VulkanGraphicsCard(const VulkanGraphicsCard&)=delete;
            VulkanGraphicsCard& operator=(const VulkanGraphicsCard&)=delete;
            VulkanGraphicsCard(VulkanGraphicsCard&& from);
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&& from);
            ~VulkanGraphicsCard()override;

            operator VkPhysicalDevice()const;
            operator VkDevice()const;
            VkPhysicalDevice physicalDevice();
            VkDevice device();
            VmaAllocator memoryAllocator();
            VkQueue graphicsQueue();
            VkQueue transferQueue();
            VkQueue computeQueue();
            uint32_t graphicsQueueFamily();
            uint32_t transferQueueFamily();
            uint32_t computeQueueFamily();
            const VkPhysicalDeviceProperties& properties();
            void runOneTimeCommands(VkQueue submissionQueue, uint32_t queueFamily, std::function<void(VkCommandBuffer commandBuffer)>);

            void executeArbitrary(std::function<void(CommandBuffer* commandBuffer)> execution,QueueType queue)override;


        private:
            void move(VulkanGraphicsCard&& from);
            VkPhysicalDevice _physicalDevice = nullptr;
            VkDevice _device = nullptr;
            VkQueue _graphicsQueue = nullptr;
            VkQueue _transferQueue = nullptr;
            VkQueue _computeQueue = nullptr;
            uint32_t _graphicsQueueFamily = 0;
            uint32_t _transferQueueFamily = 0;
            uint32_t _computeQueueFamily = 0;
            VmaAllocator _allocator = nullptr;
            VkPhysicalDeviceProperties _properties;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANGRAPHICSCARD_H