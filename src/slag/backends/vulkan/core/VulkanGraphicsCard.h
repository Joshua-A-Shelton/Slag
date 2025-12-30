#ifndef SLAG_VULKANGRAPHICSCARD_H
#define SLAG_VULKANGRAPHICSCARD_H
#include <vk_mem_alloc.h>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
#include "VulkanQueue.h"

#include "VkBootstrap.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanGraphicsCard: public GraphicsCard
        {
        public:
            static VulkanGraphicsCard* selected();

            VulkanGraphicsCard(VkInstance instance, const vkb::Device& device);
            VulkanGraphicsCard(VulkanGraphicsCard&)=delete;
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&)=delete;
            VulkanGraphicsCard(VulkanGraphicsCard&& from);
            VulkanGraphicsCard& operator=(VulkanGraphicsCard&& from);
            virtual ~VulkanGraphicsCard()override;
            ///Total video memory in bytes
            virtual uint64_t videoMemory()override;
            ///Name of graphics card
            virtual std::string name()override;

            ///Gets the graphics queue
            virtual GPUQueue* graphicsQueue()override;
            ///Gets the compute queue, or a default queue that processes it's commands
            virtual GPUQueue* computeQueue()override;
            ///Gets the transfer queue, or a queue that processes it's commands
            virtual GPUQueue* transferQueue()override;

            ///Alignment requirement when binding uniform buffer memory, (eg DescriptorBundle::setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t *offset*, size_t length) and similar calls with an offset must be a multiple of this number)
            virtual uint64_t uniformBufferOffsetAlignment()override;
            ///Alignment requirement when binding storage buffer memory, (eg DescriptorBundle::setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t *offset*, size_t length) and similar calls with an offset must be a multiple of this number)
            virtual uint64_t storageBufferOffsetAlignment()override;
            ///Alignment requirement when binding descriptor buffer memory
            virtual uint64_t descriptorBufferOffsetAlignment()override;

            uint64_t maxResourceDescriptorSize()const;
            uint64_t samplerDescriptorSize()const;
            uint64_t sampledTextureDescriptorSize()const;
            uint64_t storageTextureDescriptorSize()const;
            uint64_t uniformTexelBufferDescriptorSize()const;
            uint64_t storageTexelBufferDescriptorSize()const;
            uint64_t uniformBufferDescriptorSize()const;
            uint64_t storageBufferDescriptorSize()const;
            uint64_t accelerationStructureDescriptorSize()const;


            ///Defragment video memory, blocks until finished
            virtual void defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal, size_t signalCount)override;

            VkPhysicalDevice physicalDevice() const;
            VkDevice device() const;
            VmaAllocator allocator() const;

            uint32_t graphicsQueueFamily()const;
            uint32_t computeQueueFamily()const;
            uint32_t transferQueueFamily()const;
            VkQueue presentQueue()const;
        private:
            void move(VulkanGraphicsCard& from);
            VkPhysicalDevice _physicalDevice=nullptr;
            VkDevice _device=nullptr;
            VmaAllocator _allocator = nullptr;
            VkPhysicalDeviceProperties _properties;
            VkPhysicalDeviceProperties2 _deviceProperties2;
            VkPhysicalDeviceDescriptorBufferPropertiesEXT _descriptorBufferProperties;
            uint64_t _resourceDescriptorMaxSize=0;
            uint32_t _graphicsQueueFamily=0;
            uint32_t _computeQueueFamily=0;
            uint32_t _transferQueueFamily=0;
            VulkanQueue* _graphicsQueue=nullptr;
            VulkanQueue* _computeQueue=nullptr;
            VulkanQueue* _transferQueue=nullptr;
            VkQueue _presentQueue=nullptr;
            bool _seperateCompute=true;
            bool _seperateTransfer=true;
            bool _seperatePresent=true;



        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGRAPHICSCARD_H
