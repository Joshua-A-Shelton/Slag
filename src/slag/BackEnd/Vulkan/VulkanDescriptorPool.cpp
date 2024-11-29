#include "VulkanDescriptorPool.h"
#include "VulkanLib.h"
#include "VulkanDescriptorGroup.h"

namespace slag
{
    namespace vulkan
    {
        VulkanDescriptorPool::VulkanDescriptorPool(uint32_t samplers,
                                                   uint32_t sampledTextures,
                                                   uint32_t samplerAndTextureCombined,
                                                   uint32_t storageTextures,
                                                   uint32_t uniformTexelBuffers,
                                                   uint32_t storageTexelBuffers,
                                                   uint32_t uniformBuffers,
                                                   uint32_t storageBuffers,
                                                   uint32_t inputAttachments,
                                                   uint32_t accelerationStructures)
        {
            std::vector<VkDescriptorPoolSize> sizes;
            if(samplers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLER,samplers});
            }
            if(sampledTextures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,sampledTextures});
            }
            if(samplerAndTextureCombined)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,samplerAndTextureCombined});
            }
            if(storageTextures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,storageTextures});
            }
            if(uniformTexelBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,uniformTexelBuffers});
            }
            if(storageTexelBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,storageTexelBuffers});
            }
            if(uniformBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,uniformBuffers});
            }
            if(storageBuffers)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,storageBuffers});
            }
            if(inputAttachments)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,inputAttachments});
            }
            if(accelerationStructures)
            {
                sizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,accelerationStructures});
            }

            VkDescriptorPoolCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            //TODO: make this number.... correct
            info.maxSets = 10000000;//arbitrary
            info.pPoolSizes = sizes.data();
            info.poolSizeCount = sizes.size();

            auto success = vkCreateDescriptorPool(VulkanLib::card()->device(),&info, nullptr,&_pool);
        }

        VulkanDescriptorPool::~VulkanDescriptorPool()
        {
            if(_pool)
            {
                vkDestroyDescriptorPool(VulkanLib::card()->device(),_pool, nullptr);
            }
        }

        VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorPool&& from)
        {
            move(std::move(from));
        }

        VulkanDescriptorPool& VulkanDescriptorPool::operator=(VulkanDescriptorPool&& from)
        {
            move(std::move(from));
            return *this;
        }

        void VulkanDescriptorPool::move(VulkanDescriptorPool&& from)
        {
            std::swap(_pool,from._pool);
        }

        void VulkanDescriptorPool::reset()
        {
            vkResetDescriptorPool(VulkanLib::card()->device(),_pool,0);
        }

        void* VulkanDescriptorPool::makeBundleLowLevelHandle(DescriptorGroup* forGroup)
        {
            auto group = static_cast<VulkanDescriptorGroup*>(forGroup);
            auto layout = group->layout();
            VkDescriptorSet handle;
            VkDescriptorSetAllocateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            info.descriptorPool = _pool;
            info.descriptorSetCount = 1;
            info.pSetLayouts = &layout;
            vkAllocateDescriptorSets(VulkanLib::card()->device(),&info,&handle);
            return handle;
        }

    } // vulkan
} // slag