#ifndef SLAG_DX12DESCRIPTORGROUP_H
#define SLAG_DX12DESCRIPTORGROUP_H
#include <slag/Slag.h>
namespace slag
{
    namespace dx12
    {
        class DX12DescriptorGroup: public DescriptorGroup
        {
        public:
            DX12DescriptorGroup(Descriptor* descriptors, uint32_t descriptorCount);
            ~DX12DescriptorGroup()override=default;

            ///How many descriptors are in this group
            virtual uint32_t descriptorCount()override;
            ///Retrieve a descriptor with a given index
            virtual const Descriptor& descriptor(uint32_t index)override;
            ///Get the byte offset from the beginning of a descriptor set for the descriptor at the given index
            virtual uint64_t descriptorByteOffset(uint32_t index)override;
            ///If this group is swap compatible with another group
            virtual bool compatible(DescriptorGroup* with)override;
        private:
            void copy(const DX12DescriptorGroup& from);
            void move(DX12DescriptorGroup& from);
        };
    } // dx12
} // slag

#endif //SLAG_DX12DESCRIPTORGROUP_H
