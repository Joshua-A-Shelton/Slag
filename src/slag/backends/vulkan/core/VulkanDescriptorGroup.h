#ifndef SLAG_VULKANDESCRIPTORGROUP_H
#define SLAG_VULKANDESCRIPTORGROUP_H
#include <slag/Slag.h>
#include <vulkan/vulkan.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanDescriptorGroup: public DescriptorGroup
        {
        public:
            VulkanDescriptorGroup()=default;
            VulkanDescriptorGroup(Descriptor* descriptors, size_t descriptorCount);
            ~VulkanDescriptorGroup()override;
            VulkanDescriptorGroup(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup& operator=(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup(VulkanDescriptorGroup&& from);
            VulkanDescriptorGroup& operator=(VulkanDescriptorGroup&& from);

            virtual uint32_t descriptorCount()override;
            virtual Descriptor& operator[](size_t index)override;
            virtual Descriptor& descriptor(size_t index)override;
            virtual VkDescriptorSetLayout layout()const;
        private:
            void copy(const VulkanDescriptorGroup& from);
            void move(VulkanDescriptorGroup& from);
            VkDescriptorSetLayout _layout = nullptr;
            std::vector<Descriptor> _bindings;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORGROUP_H
