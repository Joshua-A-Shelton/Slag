#include <cassert>
#include <algorithm>
#include "VulkanUniformSet.h"
#include "VulkanDescriptorSetLayoutCache.h"

namespace slag
{
    namespace vulkan
    {
        VulkanUniformSet::VulkanUniformSet(uint32_t index)
        {
            _index = index;
        }

        VulkanUniformSet::VulkanUniformSet(const SpvReflectDescriptorSet *description, VkShaderStageFlagBits stage)
        {
            _accessibleFrom = stage;
            _index = description->set;
            for(uint32_t bindingIndex=0; bindingIndex < description->binding_count; bindingIndex++)
            {
                auto binding = description->bindings[bindingIndex];
                _uniforms.push_back(VulkanUniform(binding, stage));
            }
            auto layout = layoutInfo();
            _descriptorSetLayout = VulkanDescriptorSetLayoutCache::getLayout(layout);
            setDynamicOffsets();
        }

        uint32_t VulkanUniformSet::index()
        {
            return _index;
        }

        size_t VulkanUniformSet::bufferCount()
        {
            return _uniforms.size();
        }

        Uniform *VulkanUniformSet::operator[](size_t index)
        {
            return &_uniforms[index];
        }

        VulkanUniformLayoutInfo VulkanUniformSet::layoutInfo()
        {
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            for(int i=0; i< _uniforms.size(); i++)
            {
                bindings.push_back(_uniforms[i].vulkanBindingInfo());
            }

            return VulkanUniformLayoutInfo(std::move(bindings));
        }

        void VulkanUniformSet::setDynamicOffsets()
        {
            _dynamicOffsets.clear();
            for(auto& uniform: _uniforms)
            {
                if(uniform.vulkanDescriptorType() == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || uniform.vulkanDescriptorType() == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
                {
                    _dynamicOffsets.push_back(uniform.binding());
                }
            }
        }

        VulkanUniformSet::VulkanUniformSet(VulkanUniformSet&& from)
        {
            move(std::move(from));
        }

        VulkanUniformSet& VulkanUniformSet::operator=(VulkanUniformSet&& from)
        {
            move(std::move(from));
            return *this;
        }

        VulkanUniformSet::~VulkanUniformSet()
        {
            if(_descriptorSetLayout)
            {
                auto layout = layoutInfo();
                VulkanDescriptorSetLayoutCache::unregisterInstance(layout);
            }
        }

        VkDescriptorSetLayout VulkanUniformSet::descriptorSetLayout()
        {
            return _descriptorSetLayout;
        }

        void VulkanUniformSet::merge(VulkanUniformSet &&with)
        {
            assert(_index == with._index && "Unable to merge uniform sets with different indexes");

            //we have to clear the current layout info for both, as they're both about to change
            auto layout = layoutInfo();
            VulkanDescriptorSetLayoutCache::unregisterInstance(layout);
            auto withLayout = with.layoutInfo();
            VulkanDescriptorSetLayoutCache::unregisterInstance(withLayout);
            _descriptorSetLayout = nullptr;
            with._descriptorSetLayout = nullptr;


            _accessibleFrom = static_cast<VkShaderStageFlagBits>(_accessibleFrom | with._accessibleFrom);
            for(size_t i=0; i< with._uniforms.size(); i++)
            {
                auto& mergingUniform = with._uniforms[i];
                bool alreadyExists = false;
                for(int j=0; j< _uniforms.size(); j++)
                {
                    if(_uniforms[j].binding() == mergingUniform.binding())
                    {
                        _uniforms[j].merge(std::move(mergingUniform));
                        alreadyExists = true;
                        break;
                    }
                }
                if(!alreadyExists)
                {
                    _uniforms.push_back(std::move(mergingUniform));
                }
            }
            std::sort(_uniforms.begin(), _uniforms.end(),VulkanUniform::compareBinding);

            //set the new layout info
            layout = layoutInfo();
            _descriptorSetLayout = VulkanDescriptorSetLayoutCache::getLayout(layout);
            setDynamicOffsets();
        }

        void VulkanUniformSet::move(VulkanUniformSet&& from)
        {
            std::swap(_index,from._index);
            std::swap(_accessibleFrom,from._accessibleFrom);
            std::swap(_descriptorSetLayout, from._descriptorSetLayout);
            _uniforms.swap(from._uniforms);
            _dynamicOffsets.swap(from._dynamicOffsets);
        }
    } // slag
} // vulkan