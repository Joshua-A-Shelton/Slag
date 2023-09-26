#ifndef SLAG_VULKANDESCRIPTORALLOCATOR_H
#define SLAG_VULKANDESCRIPTORALLOCATOR_H
#include <vector>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanDescriptorAllocator
        {
        public:
            VulkanDescriptorAllocator();
            VulkanDescriptorAllocator(const VulkanDescriptorAllocator&)=delete;
            VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&)=delete;
            VulkanDescriptorAllocator& operator=(VulkanDescriptorAllocator&& from);
            ~VulkanDescriptorAllocator();

            struct PoolSizes {
                std::vector<std::pair<VkDescriptorType,float>> sizes =
                        {
                                { VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
                                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
                                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
                                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
                                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
                                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
                                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
                                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
                                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
                                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
                                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
                        };
            };

            void resetPools();
            bool allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);


        private:
            VkDescriptorPool grab_pool();
            VkDescriptorPool createPool(VkDevice device, const VulkanDescriptorAllocator::PoolSizes& poolSizes, int count, VkDescriptorPoolCreateFlags flags);
            VkDescriptorPool currentPool{VK_NULL_HANDLE};
            PoolSizes descriptorSizes;
            std::vector<VkDescriptorPool> usedPools;
            std::vector<VkDescriptorPool> freePools;
        };
    } // slag
} // vulkan
#endif //SLAG_VULKANDESCRIPTORALLOCATOR_H