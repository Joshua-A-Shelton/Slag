#include "DX12Shader.h"

namespace slag
{
    namespace dx
    {
        size_t DX12Shader::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* DX12Shader::descriptorGroup(size_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* DX12Shader::operator[](size_t index)
        {
            return &_descriptorGroups[index];
        }

        size_t DX12Shader::pushConstantRangeCount()
        {
            return _pushConstantRanges.size();
        }

        PushConstantRange DX12Shader::pushConstantRange(size_t index)
        {
            return _pushConstantRanges.at(index);
        }
    } // dx
} // slag