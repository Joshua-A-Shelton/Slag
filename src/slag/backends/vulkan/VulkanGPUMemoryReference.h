#ifndef SLAG_VULKANGPUMEMORYREFERENCE_H
#define SLAG_VULKANGPUMEMORYREFERENCE_H

namespace slag
{
    namespace vulkan
    {
        class VulkanBuffer;
        class VulkanTexture;

        union VulkanGPUMemory
        {
            VulkanBuffer* buffer;
            VulkanTexture* texture;
        };
        enum class MemoryType
        {
            BUFFER,
            TEXTURE
        };
        struct VulkanGPUMemoryReference
        {
            MemoryType memoryType;
            VulkanGPUMemory reference;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANGPUMEMORYREFERENCE_H
