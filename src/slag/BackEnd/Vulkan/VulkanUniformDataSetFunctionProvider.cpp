#include "VulkanUniformDataSetFunctionProvider.h"
#include "VulkanDescriptorAllocator.h"
#include "VulkanUniformSet.h"
#include "VulkanLib.h"
#include "VulkanTexture.h"
#include "VulkanTextureSampler.h"

namespace slag
{
    namespace vulkan
    {

        void VulkanUniformDataSetFunctionProvider::initializeLowLevelHandle(void **lowLevelHandle, UniformSet *set, UniformSetDataAllocator *allocator)
        {
            auto vulkanAllocator = static_cast<VulkanDescriptorAllocator*>(allocator);
            auto uniformSet = static_cast<VulkanUniformSet*>(set);
            VkDescriptorSet descriptorSet;
            vulkanAllocator->allocate(&descriptorSet,uniformSet->descriptorSetLayout());
            *lowLevelHandle = descriptorSet;
        }

        void VulkanUniformDataSetFunctionProvider::setUniformBufferData(void *lowLevelHandle, uint32_t uniformIndex, BufferWriteData *writtenData)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(lowLevelHandle);
            VkDescriptorBufferInfo buffer_info =
            {
                    static_cast<VkBuffer>(writtenData->backingBuffer()),
                    writtenData->location(),
                    writtenData->size()
            };

            VkWriteDescriptorSet writeSet
            {
                    VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    nullptr,
                    descriptorSet,
                    uniformIndex,
                    0,
                    1,
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    nullptr,
                    &buffer_info,
            };
            vkUpdateDescriptorSets(VulkanLib::graphicsCard()->device(),1,&writeSet,0, nullptr);
        }

        void VulkanUniformDataSetFunctionProvider::setTexture(void* lowLevelHandle, uint32_t uniformIndex, Texture* texture, TextureSampler* sampler, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(lowLevelHandle);
            auto image = static_cast<VulkanTexture*>(texture);
            auto texSampler = static_cast<VulkanTextureSampler*>(sampler);
            auto vulkanLayout = VulkanTexture::layoutFromCrossPlatform(layout);
            VkDescriptorImageInfo image_info =
                    {
                            texSampler->vulkanSampler(),
                            image->vulkanView(),
                            vulkanLayout
                    };
            VkWriteDescriptorSet writeSet
                    {
                            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            nullptr,
                            descriptorSet,
                            uniformIndex,
                            0,
                            1,
                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                            &image_info,
                            nullptr,
                            nullptr
                    };
            vkUpdateDescriptorSets(VulkanLib::graphicsCard()->device(),1,&writeSet,0, nullptr);
        }

        void VulkanUniformDataSetFunctionProvider::setImage(void* lowLevelHandle, uint32_t uniformIndex, Texture* texture, Texture::Layout layout)
        {
            VkDescriptorSet descriptorSet = static_cast<VkDescriptorSet>(lowLevelHandle);
            auto image = static_cast<VulkanTexture*>(texture);
            auto vulkanLayout = VulkanTexture::layoutFromCrossPlatform(layout);
            VkDescriptorImageInfo image_info =
                    {
                            nullptr,
                            image->vulkanView(),
                            vulkanLayout
                    };
            VkWriteDescriptorSet writeSet
                    {
                            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                            nullptr,
                            descriptorSet,
                            uniformIndex,
                            0,
                            1,
                            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                            &image_info,
                            nullptr,
                            nullptr
                    };
            vkUpdateDescriptorSets(VulkanLib::graphicsCard()->device(),1,&writeSet,0, nullptr);
        }
    } // slag
} // vulkan