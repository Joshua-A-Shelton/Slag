#ifndef SLAG_VULKANDESCRIPTORGROUPCACHE_H
#define SLAG_VULKANDESCRIPTORGROUPCACHE_H

#include <mutex>
#include <unordered_map>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanDescriptorGroup;

        class CachedLayout
        {
        public:
            VkDescriptorSetLayout layout = nullptr;
            uint64_t instanceCount = 0;
        };

        class VulkanDescriptorGroupCache
        {
        public:
            static VkDescriptorSetLayout getLayout(VulkanDescriptorGroup& group);
            static void removeInstance(VulkanDescriptorGroup& group);
        private:
            static inline std::mutex _cacheMutex;
            static inline std::unordered_map<DescriptorGroup::Shape, CachedLayout,DescriptorGroup::Shape::DescriptorGroupShapeHash> _cachedLayouts;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORGROUPCACHE_H
