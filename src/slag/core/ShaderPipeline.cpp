#include "ShaderPipeline.h"

#include "GraphicsCard.h"

namespace slag
{
    ShaderParameterTable::ShaderParameterTable(const BindGroup& group, const GraphicsCard* graphicsCard)
    {
        _bindingIndex = group.bindIndex();
        _entries.reserve(group.descriptorInfoCount());
        uint32_t offset = 0;
        auto& offsetData = graphicsCard->descriptorTableDetails();
        for (auto i=0u; i<group.descriptorInfoCount(); ++i)
        {
            auto& descriptorInfo = group.descriptorInfo(i);
            uint32_t alignment = 0;
            uint32_t size = 0;
            switch (descriptorInfo.type())
            {
            case DescriptorType::SAMPLER:
                alignment = offsetData.samplerDescriptorAlignment;
                size = offsetData.samplerDescriptorSize;
                break;
            case DescriptorType::SAMPLED_TEXTURE:
                alignment = offsetData.sampledTextureAlignment;
                size = offsetData.sampledTextureSize;
                break;
            case DescriptorType::UNORDERED_ACCESS_TEXTURE:
                alignment = offsetData.unorderedAccessTextureAlignment;
                size = offsetData.unorderedAccessTextureSize;
                break;
            case DescriptorType::UNIFORM_BUFFER:
                alignment = offsetData.uniformBufferAlignment;
                size = offsetData.uniformBufferSize;
                break;
            case DescriptorType::UNORDERED_ACCESS_BUFFER:
                alignment = offsetData.unorderedAccessBufferAlignment;
                size = offsetData.unorderedAccessBufferSize;
                break;
            case DescriptorType::UNIFORM_TEXEL_BUFFER:
                alignment = offsetData.uniformTexelBufferAlignment;
                size = offsetData.uniformTexelBufferSize;
                break;
            case DescriptorType::UNORDERED_ACCESS_TEXEL_BUFFER:
                alignment = offsetData.unorderedAccessTexelBufferAlignment;
                size = offsetData.unorderedAccessTexelBufferSize;
                break;
            case DescriptorType::ACCELERATION_STRUCTURE:
                alignment = offsetData.accelerationStructureAlignment;
                size = offsetData.accelerationStructureSize;
                break;
            }
            _entries.emplace_back(descriptorInfo,(offset +(alignment-1)) & ~(alignment-1));
            offset+=size;
        }
    }

    ShaderParameterTable::ShaderParameterTable(ShaderParameterTable&& from) noexcept
    {
        move(from);
    }

    ShaderParameterTable& ShaderParameterTable::operator=(ShaderParameterTable&& from) noexcept
    {
        move(from);
        return *this;
    }

    uint32_t ShaderParameterTable::bindingIndex() const
    {
        return _bindingIndex;
    }

    uint32_t ShaderParameterTable::entryCount() const
    {
        return _entries.size();
    }

    const ShaderTableEntry& ShaderParameterTable::entry(uint32_t index) const
    {
        return _entries[index];
    }

    void ShaderParameterTable::move(ShaderParameterTable& from)
    {
        _bindingIndex = from._bindingIndex;
        _entries.swap(from._entries);
    }
}
