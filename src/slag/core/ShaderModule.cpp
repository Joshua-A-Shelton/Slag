#include "ShaderModule.h"

#include "slag/Slag.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    BindGroup::BindGroup(uint32_t bindIndex, std::vector<DescriptorMeta>&& descriptorInfo)
    {
        SLAG_ASSERT(descriptorInfo.size() > 0 && "Cannot have an empty bind group");
        SLAG_ASSERT(descriptorInfo.size() <= std::numeric_limits<uint32_t>::max() && "Too many descriptors provided for bind group");
        _bindIndex = bindIndex;
        _descriptorInfo = descriptorInfo;
    }

    uint32_t BindGroup::bindIndex() const
    {
        return _bindIndex;
    }

    uint32_t BindGroup::descriptorInfoCount() const
    {
        return _descriptorInfo.size();
    }

    const DescriptorMeta& BindGroup::descriptorInfo(uint32_t index) const
    {
        return _descriptorInfo[index];
    }

    InputVariable::InputVariable(const std::string& name, GraphicsType type, uint32_t location)
    {
        _name = name;
        _type = type;
        _location = location;
    }

    const std::string& InputVariable::name() const
    {
        return _name;
    }

    GraphicsType InputVariable::type() const
    {
        return _type;
    }

    uint32_t InputVariable::location() const
    {
        return _location;
    }

    ShaderMetaData::ShaderMetaData(ShaderType type, std::vector<BindGroup>&& bindGroups, std::vector<InputVariable>&& inputVariables)
    {
        SLAG_ASSERT(bindGroups.size() <= std::numeric_limits<uint32_t>::max() && "Too many inputs provided for shader meta data");
        SLAG_ASSERT(inputVariables.size() <= std::numeric_limits<uint32_t>::max() && "Too many inputs provided for input variables");
        _type = type;
        _bindGroups = std::move(bindGroups);
        _inputVariables = std::move(inputVariables);
    }

    ShaderType ShaderMetaData::type() const
    {
        return _type;
    }

    uint32_t ShaderMetaData::bindGroupCount() const
    {
        return _bindGroups.size();
    }

    const BindGroup& ShaderMetaData::bindGroup(uint32_t index)const
    {
        SLAG_ASSERT(index < _bindGroups.size() && "Index out of bounds");
        return _bindGroups[index];
    }

    const uint32_t ShaderMetaData::inputVariableCount() const
    {
        return _inputVariables.size();
    }

    const InputVariable& ShaderMetaData::inputVariable(uint32_t index) const
    {
        SLAG_ASSERT(index < _inputVariables.size() && "Index out of bounds");
        return _inputVariables[index];
    }
} // slag
