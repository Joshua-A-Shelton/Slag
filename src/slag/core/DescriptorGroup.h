#ifndef SLAG_DESCRIPTORGROUP_H
#define SLAG_DESCRIPTORGROUP_H

#include "Descriptor.h"
namespace slag
{
    class DescriptorIndex
    {
    };
    ///Describes the expected collection of descriptors required in a shader at a group level
    class DescriptorGroup
    {
    public:

        virtual ~DescriptorGroup()=default;
        ///How many descriptors are in this group
        virtual uint32_t descriptorCount()=0;
        /**
         * Get the name of a descriptor
         * @param index number between 0 and descriptorCount
         * @return
         */
        virtual std::string descriptorName(uint32_t index)=0;
        /**
         * Get the index of a descriptor with a name
         * @param descriptorName Name of the descriptor to get the index for
         * @return
         */
        virtual DescriptorIndex* indexOf(const std::string& descriptorName)=0;
        ///Get descriptor at the given index
        virtual Descriptor* descriptor(DescriptorIndex* index)=0;
        ///Get descriptor with the given name
        virtual Descriptor* descriptor(const std::string& descriptorName)=0;
        ///If this group is swap compatible with another group
        virtual bool compatible(DescriptorGroup* with)=0;


    };
}

#endif //SLAG_DESCRIPTORGROUP_H