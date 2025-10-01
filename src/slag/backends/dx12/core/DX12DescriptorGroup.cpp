#include "DX12DescriptorGroup.h"

namespace slag
{
    namespace dx12
    {


        DX12DescriptorGroup::DX12DescriptorGroup(Descriptor* descriptors, uint32_t descriptorCount)
        {
            int i=0;
        }

        uint32_t DX12DescriptorGroup::descriptorCount()
        {
            throw std::runtime_error("DX12DescriptorGroup::descriptorCount() is not implemented");
            //return _bindings.size();
        }

        std::string DX12DescriptorGroup::descriptorName(uint32_t index)
        {
            return _descriptorNames[index];
        }

        void DX12DescriptorGroup::copy(const DX12DescriptorGroup& from)
        {
            throw std::runtime_error("DX12DescriptorGroup::copy() is not implemented");
            //_bindings = from._bindings;
        }

        void DX12DescriptorGroup::move(DX12DescriptorGroup& from)
        {
            throw std::runtime_error("DX12DescriptorGroup::move() is not implemented");
            //_bindings = std::move(from._bindings);
        }
    } // dx12
} // slag
