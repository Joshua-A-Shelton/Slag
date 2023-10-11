#include "VulkanDescriptorSetLayoutCache.h"
#include "VulkanLib.h"
namespace slag
{
    namespace vulkan
    {
        VkDescriptorSetLayout VulkanDescriptorSetLayoutCache::getLayout(VulkanUniformLayoutInfo &info)
        {
            if(info.bindings().size()==0)
            {
                return nullptr;
            }
            auto hash = info.hash();
            auto layout = _cachedLayouts.find(hash);
            if(layout == _cachedLayouts.end())
            {
                CachedLayout cached{};
                cached.instanceCount = 1;
                auto result = vkCreateDescriptorSetLayout(VulkanLib::graphicsCard()->device(), &info.layoutInfo(), nullptr, &cached.layout);
                _cachedLayouts.insert(std::make_pair(hash,cached));
                return cached.layout;
            }
            else
            {
                layout->second.instanceCount++;
                return layout->second.layout;
            }
        }

        void VulkanDescriptorSetLayoutCache::unregisterInstance(VulkanUniformLayoutInfo &info)
        {
            if(info.bindings().size()==0)
            {
                return;
            }
            auto hash = info.hash();
            auto layout = _cachedLayouts.find(hash);
            if(layout != _cachedLayouts.end())
            {
                layout->second.instanceCount--;
                if(layout->second.instanceCount == 0)
                {
                    vkDestroyDescriptorSetLayout(VulkanLib::graphicsCard()->device(),layout->second.layout, nullptr);
                    _cachedLayouts.erase(hash);
                }
            }
        }
    } // slag
} // vulkan