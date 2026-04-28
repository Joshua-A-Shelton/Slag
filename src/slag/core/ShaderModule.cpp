#include "ShaderModule.h"

#include <limits>
#include <map>
#include <stdexcept>

#include "slag/Slag.h"
#include "slag/exceptions/IncompatibleShaderBindGroupsError.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    BindGroup::BindGroup(uint32_t groupIndex, std::vector<DescriptorMeta>&& descriptorInfo)
    {
        SLAG_ASSERT(descriptorInfo.size() > 0 && "Cannot have an empty bind group");
        SLAG_ASSERT(descriptorInfo.size() <= std::numeric_limits<uint32_t>::max() && "Too many descriptors provided for bind group");
        _groupIndex = groupIndex;
        _descriptorInfo = descriptorInfo;
    }

    uint32_t BindGroup::groupIndex() const
    {
        return _groupIndex;
    }

    uint32_t BindGroup::descriptorInfoCount() const
    {
        return _descriptorInfo.size();
    }

    const DescriptorMeta& BindGroup::descriptorInfo(uint32_t index) const
    {
        return _descriptorInfo[index];
    }

    ShaderInterfaceVariable::ShaderInterfaceVariable(const std::string& name, GraphicsType type, uint32_t location)
    {
        _name = name;
        _type = type;
        _location = location;
    }

    const std::string& ShaderInterfaceVariable::name() const
    {
        return _name;
    }

    GraphicsType ShaderInterfaceVariable::type() const
    {
        return _type;
    }

    uint32_t ShaderInterfaceVariable::location() const
    {
        return _location;
    }

    ShaderMetaData::ShaderMetaData(ShaderType type, std::vector<BindGroup>&& bindGroups, std::vector<ShaderInterfaceVariable>&& inputVariables, std::vector<ShaderInterfaceVariable>&& outputVariables)
    {
        SLAG_ASSERT(bindGroups.size() <= std::numeric_limits<uint32_t>::max() && "Too many inputs provided for shader meta data");
        SLAG_ASSERT(inputVariables.size() <= std::numeric_limits<uint32_t>::max() && "Too many inputs provided for input variables");
        SLAG_ASSERT(outputVariables.size() <= std::numeric_limits<uint32_t>::max() && "Too many inputs provided for output variables");
        _type = type;
        _bindGroups = std::move(bindGroups);
        _inputVariables = std::move(inputVariables);
        _outputVariables = std::move(outputVariables);
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

    uint32_t ShaderMetaData::inputVariableCount() const
    {
        return _inputVariables.size();
    }

    const ShaderInterfaceVariable& ShaderMetaData::inputVariable(uint32_t index) const
    {
        SLAG_ASSERT(index < _inputVariables.size() && "Index out of bounds");
        return _inputVariables[index];
    }

    uint32_t ShaderMetaData::outputVariableCount() const
    {
        return _outputVariables.size();
    }

    const ShaderInterfaceVariable& ShaderMetaData::outputVariable(uint32_t index) const
    {
        SLAG_ASSERT(index < _outputVariables.size() && "Index out of bounds");
        return _outputVariables[index];
    }
} // slag
