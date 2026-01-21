#include "ShaderCode.h"

#include <fstream>

#include "slag/utilities/SLAG_ASSERT.h"
#ifdef USE_SLAG_SPIRV_REFLECTION
#include "slag/spirv/SPIRVReflection.h"
#endif
#ifdef USE_SLAG_DXIL_12_REFLECTION
#include "slag/dxil_12/DXIL12Reflection.h"
#endif

namespace slag
{
    VertexInputAttribute::VertexInputAttribute(const std::string& semanticName, GraphicsType type,
        uint32_t arrayLength, uint64_t inputID)
    {
        SLAG_ASSERT(semanticName.size() > 0 &&  "VertexAttribute must have semantic name");
        SLAG_ASSERT(type != GraphicsType::STRUCT && type != GraphicsType::UNKNOWN && "Type must be a primitive graphics type");
        _semanticName = semanticName;
        _type = type;
        _arrayLength = arrayLength;
        _inputID = inputID;
    }

    const std::string& VertexInputAttribute::semanticName() const
    {
        return _semanticName;
    }

    GraphicsType VertexInputAttribute::type() const
    {
        return _type;
    }

    uint32_t VertexInputAttribute::arrayLength() const
    {
        return _arrayLength;
    }

    uint64_t VertexInputAttribute::inputID() const
    {
        return _inputID;
    }

    void DescriptorBinding::copy(DescriptorBinding& other)
    {
        _descriptor = other._descriptor;
        _bindingId = other._bindingId;
    }

    DescriptorBindingGroup::DescriptorBindingGroup(DescriptorBinding* bindings,uint32_t bindingCount, uint32_t groupIndex)
    {
        _bindings.resize(bindingCount);
        for (int i=0; i<bindingCount; i++)
        {
            _bindings[i] = bindings[i];
        }
        _descriptorGroupIndex = groupIndex;
    }

    DescriptorBinding::DescriptorBinding(Descriptor descriptor, uint64_t bindingId)
    {
        _descriptor = descriptor;
        _bindingId = bindingId;
    }

    const Descriptor& DescriptorBinding::descriptor() const
    {
        return _descriptor;
    }

    uint64_t DescriptorBinding::bindingId() const
    {
        return _bindingId;
    }

    DescriptorBindingGroup::DescriptorBindingGroup(DescriptorBindingGroup&& from)
    {
        move(from);
    }

    DescriptorBindingGroup& DescriptorBindingGroup::operator=(DescriptorBindingGroup&& from)
    {
        move(from);
        return *this;
    }

    uint32_t DescriptorBindingGroup::descriptorGroupIndex() const
    {
        return _descriptorGroupIndex;
    }

    uint32_t DescriptorBindingGroup::bindingCount() const
    {
        return _bindings.size();
    }

    const DescriptorBinding& DescriptorBindingGroup::descriptorBinding(uint32_t index) const
    {
        return _bindings[index];
    }

    void DescriptorBindingGroup::move(DescriptorBindingGroup& from)
    {
        _descriptorGroupIndex = from._descriptorGroupIndex;
        _bindings.swap(from._bindings);
    }

    BufferDescriptorBindingLayout::BufferDescriptorBindingLayout(uint32_t descriptorGroupIndex, uint32_t descriptorIndex,
        BufferLayout&& layout)
    {
        _descriptorBindingGroupIndex = descriptorGroupIndex;
        _descriptorIndex = descriptorIndex;
        _bufferLayout = std::move(layout);
    }

    uint32_t BufferDescriptorBindingLayout::descriptorGroupIndex() const
    {
        return _descriptorBindingGroupIndex;
    }

    uint32_t BufferDescriptorBindingLayout::descriptorIndex() const
    {
        return _descriptorIndex;
    }

    const BufferLayout& BufferDescriptorBindingLayout::bufferLayout() const
    {
        return _bufferLayout;
    }

    TexelBufferDescriptorBinding::TexelBufferDescriptorBinding(uint32_t descriptorGroupIndex, uint32_t descriptorIndex,
        TexelBufferDescription&& description): _descriptorBindingGroupIndex(descriptorGroupIndex), _descriptorIndex(descriptorIndex), _bufferDescription(std::move(description))
    {
    }

    uint32_t TexelBufferDescriptorBinding::descriptorGroupIndex() const
    {
        return _descriptorBindingGroupIndex;
    }

    uint32_t TexelBufferDescriptorBinding::descriptorIndex() const
    {
        return _descriptorIndex;
    }

    const TexelBufferDescription& TexelBufferDescriptorBinding::bufferDescription() const
    {
        return _bufferDescription;
    }

    ShaderMetaData::ShaderMetaData(ShaderStageFlags stage,
            std::vector<VertexInputAttribute>&& vertexInputs,
            std::vector<DescriptorBindingGroup>&& bindingGroups,
            std::vector<BufferDescriptorBindingLayout>&& uniformLayouts,
            std::vector<BufferDescriptorBindingLayout>&& storageLayouts,
            std::vector<TexelBufferDescriptorBinding>&& texelLayouts,
            BufferLayout pushConstantLayout,
            uint32_t xComputeThreads,
            uint32_t yComputeThreads,
            uint32_t zComputeThreads)
    {
        SLAG_ASSERT(std::popcount(static_cast<uint16_t>(stage)) ==1 && "Only a single stage can be assigned per shader");
        _stage = stage;
        _vertexInputs = std::move(vertexInputs);
        _bindingGroups = std::move(bindingGroups);
        _uniformBufferLayouts = std::move(uniformLayouts);
        _storageBufferLayouts = std::move(storageLayouts);
        _texelBufferDescriptions = std::move(texelLayouts);
        _pushConstantLayout = std::move(pushConstantLayout);
        _xComputeThreads = xComputeThreads;
        _yComputeThreads = yComputeThreads;
        _zComputeThreads = zComputeThreads;
    }


    ShaderMetaData::ShaderMetaData(ShaderMetaData&& from)
    {
        move(from);
    }

    ShaderMetaData& ShaderMetaData::operator=(ShaderMetaData&& from)
    {
        move(from);
        return *this;
    }

    uint32_t ShaderMetaData::inputCount() const
    {
        return _vertexInputs.size();
    }

    const VertexInputAttribute& ShaderMetaData::inputVariable(uint32_t index) const
    {
        return _vertexInputs[index];
    }

    uint32_t ShaderMetaData::descriptorBindingGroupCount() const
    {
        return _bindingGroups.size();
    }

    const DescriptorBindingGroup& ShaderMetaData::descriptorBindingGroup(uint32_t index) const
    {
        return _bindingGroups[index];
    }

    uint32_t ShaderMetaData::uniformBufferLayoutCount() const
    {
        return _uniformBufferLayouts.size();
    }

    const BufferDescriptorBindingLayout& ShaderMetaData::uniformBufferLayout(uint32_t index) const
    {
        return _uniformBufferLayouts[index];
    }

    uint32_t ShaderMetaData::storageBufferLayoutCount() const
    {
        return _storageBufferLayouts.size();
    }

    const BufferDescriptorBindingLayout& ShaderMetaData::storageBufferLayout(uint32_t index) const
    {
        return _storageBufferLayouts[index];
    }

    uint32_t ShaderMetaData::texelBufferDescriptionCount() const
    {
        return _texelBufferDescriptions.size();
    }

    const TexelBufferDescriptorBinding& ShaderMetaData::texelBufferDescription(uint32_t index) const
    {
        return _texelBufferDescriptions[index];
    }

    BufferLayout ShaderMetaData::pushConstantLayout() const
    {
        return _pushConstantLayout;
    }

    const BufferDescriptorBindingLayout* ShaderMetaData::uniformBufferLayout(uint32_t bindingGroup, uint32_t descriptorIndex) const
    {
        for (int i=0; i<_uniformBufferLayouts.size(); i++)
        {
            auto& bufferLayout = _uniformBufferLayouts[i];
            if (bufferLayout.descriptorGroupIndex() == bindingGroup && bufferLayout.descriptorIndex() == descriptorIndex)
            {
                return &bufferLayout;
            }
        }
        return nullptr;
    }

    const BufferDescriptorBindingLayout* ShaderMetaData::storageBufferLayout(uint32_t bindingGroup, uint32_t descriptorIndex) const
    {
        for (int i=0; i<_storageBufferLayouts.size(); i++)
        {
            auto& bufferLayout = _storageBufferLayouts[i];
            if (bufferLayout.descriptorGroupIndex() == bindingGroup && bufferLayout.descriptorIndex() == descriptorIndex)
            {
                return &bufferLayout;
            }
        }
        return nullptr;
    }

    const TexelBufferDescriptorBinding* ShaderMetaData::texelBufferDescription(uint32_t bindingGroup, uint32_t descriptorIndex) const
    {
        for (int i=0; i<_texelBufferDescriptions.size(); i++)
        {
            auto& bufferDescription = _texelBufferDescriptions[i];
            if (bufferDescription.descriptorGroupIndex() == bindingGroup && bufferDescription.descriptorIndex() == descriptorIndex)
            {
                return &bufferDescription;
            }
        }
        return nullptr;
    }

    uint32_t ShaderMetaData::xComputeThreads() const
    {
        return _xComputeThreads;
    }

    uint32_t ShaderMetaData::yComputeThreads() const
    {
        return _yComputeThreads;
    }

    uint32_t ShaderMetaData::zComputeThreads() const
    {
        return _zComputeThreads;
    }

    ShaderStageFlags ShaderMetaData::stage() const
    {
        return _stage;
    }

    void ShaderMetaData::move(ShaderMetaData& from)
    {
        _stage = from._stage;
        _vertexInputs.swap(from._vertexInputs);
        _bindingGroups.swap(from._bindingGroups);
        _uniformBufferLayouts.swap(from._uniformBufferLayouts);
        _storageBufferLayouts.swap(from._storageBufferLayouts);
        _texelBufferDescriptions.swap(from._texelBufferDescriptions);
        _pushConstantLayout = std::move(from._pushConstantLayout);
        _xComputeThreads = from._xComputeThreads;
        _yComputeThreads = from._yComputeThreads;
        _zComputeThreads = from._zComputeThreads;
    }

    ShaderMetaData::ShaderMetaData(ShaderCode& shaderCode)
    {
        switch (shaderCode.language())
        {
#ifdef USE_SLAG_SPIRV_REFLECTION
        case ShaderCode::CodeLanguage::SPIRV:
            *this = spirv::reflectShaderCode(shaderCode);
            break;
#endif
#ifdef USE_SLAG_DXIL_12_REFLECTION
        case ShaderCode::CodeLanguage::DXIL:
            throw std::runtime_error("ShaderMetaData::ShaderMetaData not set up for DXIL reflection");
            break;
#endif
        case ShaderCode::CodeLanguage::CUSTOM:
            if (customReflection != nullptr)
            {
                *this = customReflection(shaderCode);
                break;
            }
        default:
            throw std::invalid_argument("Shader code language not supported for reflection");
        }
    }

    ShaderCode::ShaderCode(ShaderStageFlags stage, CodeLanguage language, void* data, size_t dataLength)
    {

        SLAG_ASSERT(std::popcount((uint16_t)stage) == 1 && "Only one stage can be set per Shader Code instance");
        _stage = stage;
        _codeLanguage = language;
        _data.resize(dataLength);
        memcpy(_data.data(), data, dataLength);
    }

    ShaderCode::ShaderCode(ShaderStageFlags stage, CodeLanguage language, std::filesystem::path path)
    {
        SLAG_ASSERT(std::popcount((uint16_t)stage) == 1 && "Only one stage can be set per Shader Code instance");
        _stage = stage;
        _codeLanguage = language;
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if(!file.is_open())
        {
            throw std::runtime_error("Unable to open shader module file");
        }

        size_t size = (size_t)file.tellg();

        _data.resize(size);

        file.seekg(0);

        file.read(reinterpret_cast<std::istream::char_type*>(_data.data()), size);

        file.close();
    }

    ShaderCode::ShaderCode(ShaderCode&& from)
    {
        move(from);
    }

    ShaderCode& ShaderCode::operator=(ShaderCode&& from)
    {
        move(from);
        return *this;
    }

    ShaderCode::~ShaderCode()
    {
        if (_metaData)
        {
            delete _metaData;
        }
    }

    void* ShaderCode::data()
    {
        return _data.data();
    }

    size_t ShaderCode::dataSize() const
    {
        return _data.size();
    }

    ShaderStageFlags ShaderCode::stage() const
    {
        return _stage;
    }

    ShaderCode::CodeLanguage ShaderCode::language() const
    {
        return _codeLanguage;
    }

    ShaderMetaData* ShaderCode::metaData()
    {
        if (!_metaData)
        {
            _metaData = new ShaderMetaData(*this);
        }
        return _metaData;
    }

    void ShaderCode::move(ShaderCode& from)
    {
        _stage = from._stage;
        _codeLanguage = from._codeLanguage;
        std::swap(_data, from._data);
        std::swap(_metaData, from._metaData);
    }
} // slag
