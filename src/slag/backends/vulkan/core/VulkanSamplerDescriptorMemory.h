#ifndef SLAG_VULKANSAMPLERDESCRIPTORMEMORY_H
#define SLAG_VULKANSAMPLERDESCRIPTORMEMORY_H
#include <slag/core/SamplerDescriptorMemory.h>
#include <vulkan/vulkan.h>

#include "vk_mem_alloc.h"
#include "VulkanBuffer.h"
#include "VulkanGPUMemoryReference.h"

namespace slag
{
    class CommandBuffer;
}

namespace slag
{
    namespace vulkan
    {
        class VulkanSamplerDescriptorMemory: public SamplerDescriptorMemory
        {
        public:
            VulkanSamplerDescriptorMemory(uint64_t descriptorCount);
            ~VulkanSamplerDescriptorMemory()override;
            VulkanSamplerDescriptorMemory(const VulkanSamplerDescriptorMemory&) = delete;
            VulkanSamplerDescriptorMemory& operator=(const VulkanSamplerDescriptorMemory&) = delete;
            VulkanSamplerDescriptorMemory(VulkanSamplerDescriptorMemory&& from);
            VulkanSamplerDescriptorMemory& operator=(VulkanSamplerDescriptorMemory&& from);
            ///Size in bytes of memory
            virtual uint64_t size();
            /**
             * Get the location for the next descriptor set closest to the given memory address. Some API's require descriptor sets to be aligned to a boundary, so this call is required
             * @param memoryLocation Location in memory to check for the next aligned descriptor set location for
             * @return
             */
            virtual uint64_t descriptorGroupOffset(uint64_t memoryLocation);
            /**
             * Create a descriptor that points to a given sampler
             * @param memoryLocation Location in the memory the new descriptor will be placed
             * @param sampler Sampler the descriptor will reference
             */
            virtual void setSampler(uint64_t memoryLocation, Sampler* sampler)override;
            VulkanBufferMoveData moveMemory(VmaAllocation tempAllocation, CommandBuffer* copyDataBuffer);

            VkBuffer vulkanBuffer() const;
            VkDeviceAddress deviceAddress() const;
        private:
            void move(VulkanSamplerDescriptorMemory& from);
            uint64_t _descriptorSetAlignment = 0;
            uint64_t _size=0;
            VkBuffer _buffer = nullptr;
            VmaAllocation _allocation=nullptr;
            void* _bufferPointer=nullptr;
            VulkanGPUMemoryReference _selfReference{.memoryType = VulkanGPUMemoryReference::MemoryType::SAMPLER_DESCRIPTOR_MEMORY, .reference = {.samplerDescriptorMemory = this}};
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANSAMPLERDESCRIPTORMEMORY_H
