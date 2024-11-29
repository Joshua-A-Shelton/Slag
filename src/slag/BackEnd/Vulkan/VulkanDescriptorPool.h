#ifndef SLAG_VULKANDESCRIPTORPOOL_H
#define SLAG_VULKANDESCRIPTORPOOL_H
#include "../../DescriptorPool.h"
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {

        class VulkanDescriptorPool: public DescriptorPool
        {
        public:
            VulkanDescriptorPool(uint32_t samplers,
                                 uint32_t sampledTextures,
                                 uint32_t samplerAndTextureCombined,
                                 uint32_t storageTextures,
                                 uint32_t uniformTexelBuffers,
                                 uint32_t storageTexelBuffers,
                                 uint32_t uniformBuffers,
                                 uint32_t storageBuffers,
                                 uint32_t inputAttachments,
                                 uint32_t accelerationStructures);
            ~VulkanDescriptorPool()override;
            VulkanDescriptorPool(const VulkanDescriptorPool&)=delete;
            VulkanDescriptorPool& operator=(const VulkanDescriptorPool&)=delete;
            VulkanDescriptorPool(VulkanDescriptorPool&& from);
            VulkanDescriptorPool& operator=(VulkanDescriptorPool&& from);
            void reset()override;
            void* makeBundleLowLevelHandle(DescriptorGroup* forGroup)override;
        private:
            void move(VulkanDescriptorPool&& from);
            VkDescriptorPool _pool = nullptr;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORPOOL_H
