#ifndef SLAG_DESCRIPTORGROUP_H
#define SLAG_DESCRIPTORGROUP_H

#include "Descriptor.h"
namespace slag
{
    ///Describes the expected collection of descriptors required in a shader at a group level
    class DescriptorGroup
    {
    public:
        virtual ~DescriptorGroup()=default;
        ///How many (non-sampler) descriptors are in this group
        virtual uint32_t descriptorCount()=0;
        ///Retrieve a descriptor with a given index
        virtual const Descriptor& descriptor(uint32_t index)=0;
        ///Get the byte offset from the beginning of a descriptor set for the descriptor at the given index
        virtual uint64_t descriptorByteOffset(uint32_t index)=0;
        ///The size in a descriptor buffer this group will require (Note, the descriptors must still be aligned via GraphicsCard::descriptorBufferOffsetAlignment())
        virtual uint64_t descriptorBufferSize()=0;
        ///If this group is swap compatible with another group
        virtual bool compatible(DescriptorGroup* with)=0;




    };
}

#endif //SLAG_DESCRIPTORGROUP_H