#ifndef SLAG_VULKANGPUMEMORYREFERENCE_H
#define SLAG_VULKANGPUMEMORYREFERENCE_H

namespace slag
{
    namespace vulkan
    {
        class VulkanTexture;
        class VulkanBuffer;
        union GPUMemoryReference
        {
            VulkanTexture* texture;
            VulkanBuffer* buffer;
        };

        struct VulkanGPUMemoryReference
        {
            enum class MemoryType
            {
                TEXTURE,
                BUFFER
            };
            MemoryType memoryType;
            GPUMemoryReference reference;
        };
    }
}

#endif //SLAG_VULKANGPUMEMORYREFERENCE_H