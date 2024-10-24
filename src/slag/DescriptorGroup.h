#ifndef SLAG_DESCRIPTORGROUP_H
#define SLAG_DESCRIPTORGROUP_H

#include <cstdint>
#include "Descriptor.h"

namespace slag
{

    class DescriptorGroup
    {
    public:
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
        virtual uint32_t descriptorCount()=0;
        virtual Descriptor& operator[](size_t index)=0;

        Shape shape();
        /**
         * Manually create a descriptor group, allows for overriding descriptors found in shader via reflection
         * @param descriptors
         * @param descriptorCount
         * @return
         */
        static DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount);
    };

} // slag

#endif //SLAG_DESCRIPTORGROUP_H
