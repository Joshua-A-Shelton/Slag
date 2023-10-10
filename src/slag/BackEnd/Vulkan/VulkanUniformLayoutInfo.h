#ifndef SLAG_VULKANUNIFORMLAYOUTINFO_H
#define SLAG_VULKANUNIFORMLAYOUTINFO_H
#include <vulkan/vulkan.h>
#include <vector>
namespace slag
{
    namespace vulkan
    {

        class VulkanUniformLayoutInfo
        {
        private:
            VkDescriptorSetLayoutCreateInfo _layoutInfo{};
            std::vector<VkDescriptorSetLayoutBinding> _bindings;
            void copy(const VulkanUniformLayoutInfo& from);
            void move(VulkanUniformLayoutInfo&& from);
        public:
            VulkanUniformLayoutInfo(std::vector<VkDescriptorSetLayoutBinding>&& bindings);
            VulkanUniformLayoutInfo()=delete;
            VulkanUniformLayoutInfo(VulkanUniformLayoutInfo&& from);
            VulkanUniformLayoutInfo& operator=(VulkanUniformLayoutInfo&& from);
            VulkanUniformLayoutInfo(const VulkanUniformLayoutInfo& from);
            VulkanUniformLayoutInfo& operator=(VulkanUniformLayoutInfo& from);

            VkDescriptorSetLayoutCreateInfo& layoutInfo();

            const std::vector<VkDescriptorSetLayoutBinding>& bindings();
            size_t hash() const;
        };

    } // slag
} // vulkan

#endif //SLAG_VULKANUNIFORMLAYOUTINFO_H
