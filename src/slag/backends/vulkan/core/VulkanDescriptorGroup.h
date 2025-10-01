#ifndef SLAG_VULKANDESCRIPTORGROUP_H
#define SLAG_VULKANDESCRIPTORGROUP_H
#include <unordered_map>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

#include "VulkanDescriptorIndex.h"

namespace slag
{
    namespace vulkan
    {

        class VulkanDescriptorGroup: public DescriptorGroup
        {
        public:
            class Shape
            {
            private:
                std::vector<Descriptor::Shape> _descriptorShapes;
            public:
                friend class VulkanDescriptorGroup;
                bool operator == (const Shape& to)const;
                bool operator != (const Shape& to)const;
                struct DescriptorGroupShapeHash
                {
                    size_t operator()(const Shape& shape)const;
                };
            };

            VulkanDescriptorGroup()=default;
            VulkanDescriptorGroup(Descriptor* descriptors, uint32_t descriptorCount);
            ~VulkanDescriptorGroup()override;
            VulkanDescriptorGroup(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup& operator=(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup(VulkanDescriptorGroup&& from);
            VulkanDescriptorGroup& operator=(VulkanDescriptorGroup&& from);

            ///How many descriptors are in this group
            virtual uint32_t descriptorCount()override;
            /**
             * Get the name of a descriptor
             * @param index number between 0 and descriptorCount
             * @return
             */
            virtual std::string descriptorName(uint32_t index)override;
            /**
             * Get the index of a descriptor with a name
             * @param descriptorName Name of the descriptor to get the index for
             * @return
             */
            virtual DescriptorIndex* indexOf(const std::string& descriptorName)override;
            ///Get descriptor at the given index
            virtual Descriptor* descriptor(DescriptorIndex* index)override;
            ///Get descriptor with the given name
            virtual Descriptor* descriptor(const std::string& descriptorName)override;
            ///If this group is swap compatible with another group
            virtual bool compatible(DescriptorGroup* with)override;
            virtual VkDescriptorSetLayout layout()const;
            Shape& groupShape();
            Descriptor& descriptorAtBinding(uint32_t binding);
        private:
            void copy(const VulkanDescriptorGroup& from);
            void move(VulkanDescriptorGroup& from);
            VkDescriptorSetLayout _layout = nullptr;
            std::vector<Descriptor> _bindings;
            std::vector<std::string> _descriptorNames;
            std::unordered_map<std::string, VulkanDescriptorIndex> _descriptorIndexMap;
            Shape _groupShape;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORGROUP_H
