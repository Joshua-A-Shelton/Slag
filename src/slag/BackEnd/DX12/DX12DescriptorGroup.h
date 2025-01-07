#ifndef SLAG_DX12DESCRIPTORGROUP_H
#define SLAG_DX12DESCRIPTORGROUP_H
#include "../../DescriptorGroup.h"
#include <directx/d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12DescriptorGroup: public DescriptorGroup
        {
        public:
            DX12DescriptorGroup(Descriptor* descriptors, size_t descriptorCount);
            ~DX12DescriptorGroup()override=default;

            uint32_t descriptorCount()override;
            Descriptor& operator[](size_t index)override;
            Descriptor& descriptor(size_t index)override;
        private:
            void copy(const DX12DescriptorGroup& from);
            void move(DX12DescriptorGroup&& from);
            std::vector<Descriptor> _bindings;
        };

    } // dx
} // slag

#endif //SLAG_DX12DESCRIPTORGROUP_H
