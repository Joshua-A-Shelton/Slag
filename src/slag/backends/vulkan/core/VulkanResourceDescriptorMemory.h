#ifndef SLAG_VULKANRESOUCEDESCRIPTORMEMORY_H
#define SLAG_VULKANRESOUCEDESCRIPTORMEMORY_H
#include <slag/core/ResourceDescriptorMemory.h>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include "VulkanBuffer.h"

namespace slag
{
    namespace vulkan
    {
        class VulkanResourceDescriptorMemory: public ResourceDescriptorMemory
        {
        public:
            VulkanResourceDescriptorMemory(uint64_t descriptorCount);
            ~VulkanResourceDescriptorMemory()override;
            VulkanResourceDescriptorMemory(const VulkanResourceDescriptorMemory&) = delete;
            VulkanResourceDescriptorMemory& operator=(const VulkanResourceDescriptorMemory&) = delete;
            VulkanResourceDescriptorMemory(VulkanResourceDescriptorMemory&& from);
            VulkanResourceDescriptorMemory& operator=(VulkanResourceDescriptorMemory&& from);
            virtual uint64_t size()override;
            virtual uint64_t handle()override;
            virtual uint64_t nextDescriptorGroupOffset(uint64_t memoryLocation)override;
            virtual void setSampledTexture(uint64_t memoryLocation, Texture* texture)override;
            virtual void setStorageTexture(uint64_t memoryLocation, Texture* texture)override;
            virtual void setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elements)override;
            virtual void setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)override;
            virtual void setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)override;
            virtual void setStorageBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t dataStride, uint64_t startIndex, uint64_t elementCount)override;

            VulkanBufferMoveData moveMemory(VmaAllocation tempAllocation, CommandBuffer* copyDataBuffer);

            VkBuffer vulkanBuffer() const;
            VkDeviceAddress deviceAddress() const;
        private:
            void move(VulkanResourceDescriptorMemory& from);
            uint64_t _descriptorSetAlignment = 0;
            uint64_t _size=0;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation=nullptr;
            void* _bufferPointer=nullptr;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::MemoryType::RESOURCE_DESCRIPTOR_MEMORY, .reference = {.resourceDescriptorMemory = this}};

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANRESOUCEDESCRIPTORMEMORY_H
