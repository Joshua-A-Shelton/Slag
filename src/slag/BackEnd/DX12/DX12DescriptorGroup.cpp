#include "DX12DescriptorGroup.h"

namespace slag
{
    namespace dx
    {
        bool compareDescriptorIndex(const Descriptor& a, const Descriptor& b)
        {
            return a.shape().binding < b.shape().binding;
        }

        DX12DescriptorGroup::DX12DescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            _bindings.resize(descriptorCount);
            size_t lastIndex = 0;
            bool sorted = true;
            for(size_t i=0; i< descriptorCount; i++)
            {
                auto desc = descriptors[i];
                if(desc.shape().binding >= lastIndex)
                {
                    lastIndex = desc.shape().binding;
                }
                else
                {
                    sorted = false;
                }
                _bindings[i] = std::move(desc);
            }
            if(!sorted)
            {
                std::sort(_bindings.begin(),_bindings.end(),compareDescriptorIndex);
            }
        }

        void DX12DescriptorGroup::copy(const DX12DescriptorGroup& from)
        {
            _bindings = from._bindings;
        }

        uint32_t DX12DescriptorGroup::descriptorCount()
        {
            return _bindings.size();
        }

        Descriptor& DX12DescriptorGroup::operator[](size_t index)
        {
            return _bindings[index];
        }

        Descriptor& DX12DescriptorGroup::descriptor(size_t index)
        {
            return _bindings.at(index);
        }

        void DX12DescriptorGroup::move(DX12DescriptorGroup&& from)
        {
            _bindings.swap(from._bindings);
        }
    } // dx
} // slag