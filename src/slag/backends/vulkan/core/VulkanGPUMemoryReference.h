#ifndef SLAG_VULKANGPUMEMORYREFERENCE_H
#define SLAG_VULKANGPUMEMORYREFERENCE_H

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;
        class VulkanBuffer;
        class VulkanResourceDescriptorMemory;
        class VulkanSamplerDescriptorMemory;
        union GPUMemoryReference
        {
            VulkanTexture* texture;
            VulkanBuffer* buffer;
            VulkanResourceDescriptorMemory* resourceDescriptorMemory;
            VulkanSamplerDescriptorMemory* samplerDescriptorMemory;
        };

        struct VulkanGPUMemoryReference
        {
            enum class MemoryType
            {
                TEXTURE,
                BUFFER,
                RESOURCE_DESCRIPTOR_MEMORY,
                SAMPLER_DESCRIPTOR_MEMORY,
            };
            MemoryType memoryType;
            GPUMemoryReference reference;
        };
    }
}

#endif //SLAG_VULKANGPUMEMORYREFERENCE_H