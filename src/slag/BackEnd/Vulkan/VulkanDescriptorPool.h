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
            VulkanDescriptorPool(const DescriptorPoolPageInfo& pageInfo);
            ~VulkanDescriptorPool()override;
            VulkanDescriptorPool(const VulkanDescriptorPool&)=delete;
            VulkanDescriptorPool& operator=(const VulkanDescriptorPool&)=delete;
            VulkanDescriptorPool(VulkanDescriptorPool&& from);
            VulkanDescriptorPool& operator=(VulkanDescriptorPool&& from);
            void reset()override;
            void setBundleLowLevelHandles(void** gpuHandle, void** cpuHandle, DescriptorGroup* forGroup) override;
        private:
            void move(VulkanDescriptorPool&& from);
            VkDescriptorPool allocatePage();
            std::vector<VkDescriptorPool> _pages;
            size_t _currentPage = 0;
            DescriptorPoolPageInfo _pageInfo;
        };

    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORPOOL_H
