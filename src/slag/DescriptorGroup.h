#ifndef SLAG_DESCRIPTORGROUP_H
#define SLAG_DESCRIPTORGROUP_H

#include <cstdint>
#include "Descriptor.h"

namespace slag
{

    ///Describes the expected collection of descriptors required in a shader at a group level
    class DescriptorGroup
    {
    public:
        ///Defines how a DescriptorGroup's descriptors are layed out. Equivalent shapes mean DescriptorBundles created from them can be assigned interchangeably
        class Shape
        {
        private:
            std::vector<Descriptor::Shape> _descriptorShapes;
        public:
            friend class DescriptorGroup;
            bool operator == (const Shape& to)const;
            bool operator != (const Shape& to)const;
            struct DescriptorGroupShapeHash
            {
                size_t operator()(const Shape& shape)const;
            };

        };

        virtual ~DescriptorGroup()=default;
        ///How many descriptors are in this group
        virtual uint32_t descriptorCount()=0;
        virtual Descriptor& operator[](size_t index)=0;
        ///Get descriptor at the given index
        virtual Descriptor& descriptor(size_t index)=0;

        Shape shape();
        /**
         * Manually create a descriptor group, allows for overriding descriptors found in shader via reflection
         * @param descriptors
         * @param descriptorCount
         * @return
         */
        static DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount);
        /**
         * Get the descriptors that are the combinations of all groups, suitable for creating a new descriptor group that encompasses all groups
         * @param combineGroups Array of groups to combine
         * @param descriptorGroupCount number of groups to combine
         * @return
         */
        static std::vector<Descriptor> combine(DescriptorGroup** combineGroups, size_t descriptorGroupCount);
    };

} // slag

#endif //SLAG_DESCRIPTORGROUP_H
