#ifndef SLAG_VULKANDESCRIPTORGROUP_H
#define SLAG_VULKANDESCRIPTORGROUP_H
#include <unordered_map>
#include <slag/Slag.h>
#include <vulkan/vulkan.h>

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
                friend class VulkanDescriptorGroupCache;
                bool operator == (const Shape& to)const;
                bool operator != (const Shape& to)const;
                struct DescriptorGroupShapeHash
                {
                    size_t operator()(const Shape& shape)const;
                };
                Shape(){}
                Shape(std::vector<Descriptor::Shape>&& shapes)
                {
                    _descriptorShapes = std::move(shapes);
                }
            };

            VulkanDescriptorGroup()=default;
            VulkanDescriptorGroup(Descriptor* descriptors, uint32_t* indexMappings, uint32_t descriptorCount, VulkanDescriptorGroup::Shape& shape);
            ~VulkanDescriptorGroup()override;
            VulkanDescriptorGroup(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup& operator=(const VulkanDescriptorGroup& from);
            VulkanDescriptorGroup(VulkanDescriptorGroup&& from);
            VulkanDescriptorGroup& operator=(VulkanDescriptorGroup&& from);

            ////How many (non-sampler) descriptors are in this group
            virtual uint32_t descriptorCount()override;
            ///Retrieve a descriptor with a given index
            virtual const Descriptor& descriptor(uint32_t index)override;
            ///Get the byte offset from the beginning of a descriptor set for the descriptor at the given index
            virtual uint64_t descriptorByteOffset(uint32_t index)override;
            ///The size in a descriptor buffer this group will require (Note, the descriptors must still be aligned via GraphicsCard::descriptorBufferOffsetAlignment())
            virtual uint64_t descriptorBufferSize()override;
            ///If this group is swap compatible with another group
            virtual bool compatible(DescriptorGroup* with)override;
            virtual VkDescriptorSetLayout layout()const;
            Shape& groupShape();
        private:
            void copy(const VulkanDescriptorGroup& from);
            void move(VulkanDescriptorGroup& from);
            VkDescriptorSetLayout _layout = nullptr;
            std::vector<Descriptor> _descriptors;
            std::vector<uint64_t> _descriptorByteOffsets;
            Shape _groupShape;
            VkDeviceSize _descriptorBufferSize = 0;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANDESCRIPTORGROUP_H
