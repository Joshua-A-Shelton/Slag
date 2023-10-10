#include "VulkanUniformSet.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniformSet::VulkanUniformSet(const SpvReflectDescriptorSet *description, VkShaderStageFlagBits stage)
        {
            _accessibleFrom = stage;
            _index = description->set;
            for(uint32_t bindingIndex=0; bindingIndex < description->binding_count; bindingIndex++)
            {
                auto binding = description->bindings[bindingIndex];
                _uniforms.push_back(Uniform(binding, stage));
            }
            auto layout = layoutInfo();
            _descriptorSetLayout = DescriptorSetLayoutCache::getLayout(layout);
            setDynamicOffsets();
        }
    } // slag
} // vulkan