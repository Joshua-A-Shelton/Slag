#ifndef SLAG_VULKANGRAPHICSCARD_H
#define SLAG_VULKANGRAPHICSCARD_H
#include "../../GraphicsCard.h"
#include <vulkan/vulkan.h>
#include <functional>
#include "vk_mem_alloc.h"
#include "VkBootstrap.h"
#include "VulkanQueue.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanGraphicsCard: public GraphicsCard
        {
        public:
            VulkanGraphicsCard(VkInstance instance,vkb::Device& device);
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

            uint32_t graphicsQueueFamily();
            uint32_t transferQueueFamily();
            uint32_t computeQueueFamily();
            const VkPhysicalDeviceProperties& properties();


            GpuQueue* graphicsQueue()override;
            GpuQueue* transferQueue()override;
            GpuQueue* computeQueue()override;
            VkQueue presentQueue();

            void defragmentMemory()override;

        private:
            void move(VulkanGraphicsCard&& from);
            VkPhysicalDevice _physicalDevice = nullptr;
            VkDevice _device = nullptr;
            VulkanQueue* _graphicsQueue = nullptr;
            VulkanQueue* _transferQueue = nullptr;
            VulkanQueue* _computeQueue = nullptr;
            VkQueue _presentQueue = nullptr;
            uint32_t _graphicsQueueFamily = 0;
            uint32_t _transferQueueFamily = 0;
            uint32_t _computeQueueFamily = 0;
            VmaAllocator _allocator = nullptr;
            VkPhysicalDeviceProperties _properties;
            bool _seperateTransfer = true;
            bool _seperateCompute = true;
        };
    } // vulkan
} // slag

#endif //CRUCIBLEEDITOR_VULKANGRAPHICSCARD_H
