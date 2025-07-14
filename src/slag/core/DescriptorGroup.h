#ifndef SLAG_DESCRIPTORGROUP_H
#define SLAG_DESCRIPTORGROUP_H

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
        ///Order and types of descriptors in group
        Shape shape();

    };
}

#endif //SLAG_DESCRIPTORGROUP_H