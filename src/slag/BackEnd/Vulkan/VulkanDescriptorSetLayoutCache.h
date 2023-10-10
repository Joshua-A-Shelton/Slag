#ifndef SLAG_VULKANDESCRIPTORSETLAYOUTCACHE_H
#define SLAG_VULKANDESCRIPTORSETLAYOUTCACHE_H
#include <vulkan/vulkan.h>
#include <unordered_map>
#include "VulkanUniformLayoutInfo.h"
namespace slag
{
    namespace vulkan
    {
        class CachedLayout
        {
        public:
            VkDescriptorSetLayout layout = nullptr;
            uint32_t instanceCount = 0;
        };

        class VulkanDescriptorSetLayoutCache
        {
        public:
            static VkDescriptorSetLayout getLayout(VulkanUniformLayoutInfo& info);
            static void unregisterInstance(VulkanUniformLayoutInfo& info);
        private:
            static inline std::unordered_map<size_t,CachedLayout> _cachedLayouts;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANDESCRIPTORSETLAYOUTCACHE_H
