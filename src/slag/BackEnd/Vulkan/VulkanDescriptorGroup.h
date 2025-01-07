#ifndef SLAG_VULKANDESCRIPTORGROUP_H
#define SLAG_VULKANDESCRIPTORGROUP_H
#include "../../DescriptorGroup.h"
#include <vulkan/vulkan.h>

namespace slag
{
    namespace vulkan
    {

        class VulkanDescriptorGroup: public DescriptorGroup
        {
        public:
            VulkanDescriptorGroup(Descriptor* descriptors, size_t descriptorCount);
            ~VulkanDescriptorGroup()override;
            VulkanDescriptorGroup(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup& operator=(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup(VulkanDescriptorGroup&& from);
            VulkanDescriptorGroup& operator=(VulkanDescriptorGroup&& from);

            uint32_t descriptorCount()override;
            Descriptor& operator[](size_t index)override;
            Descriptor& descriptor(size_t index)override;
            VkDescriptorSetLayout layout()const;

        private:
            void copy(const VulkanDescriptorGroup& from);
            void move(VulkanDescriptorGroup& from);
            VkDescriptorSetLayout _layout = nullptr;
            std::vector<Descriptor> _bindings;

        };

    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORGROUP_H
