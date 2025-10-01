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
        private:
            void copy(const DX12DescriptorGroup& from);
            void move(DX12DescriptorGroup& from);
            std::vector<std::string> _descriptorNames;
        };
    } // dx12
} // slag

#endif //SLAG_DX12DESCRIPTORGROUP_H
