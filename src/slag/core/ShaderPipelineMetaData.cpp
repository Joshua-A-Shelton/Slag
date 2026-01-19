#include "ShaderPipelineMetaData.h"

#include <unordered_map>

namespace slag {
    ShaderPipelineMetaData::ShaderPipelineMetaData(ShaderCode** stagePointers, uint32_t stageCount)
    {
        build(stagePointers, stageCount);
    }

    ShaderPipelineMetaData::ShaderPipelineMetaData(ShaderCode* stages, uint32_t stageCount)
    {
        std::vector<ShaderCode*> shaderPointers(stageCount);
        for (uint32_t i = 0; i < stageCount; i++)
        {
            shaderPointers[i] = &stages[i];
        }
        build(shaderPointers.data(), stageCount);
    }

    ShaderStageFlags ShaderPipelineMetaData::stageFlags()
    {
        return _stages;
    }

    uint32_t ShaderPipelineMetaData::vertexInputCount()
    {
        return _vertexInputs.size();
    }

    const VertexInputAttribute& ShaderPipelineMetaData::vertexInput(uint32_t index)
    {
        return _vertexInputs[index];
    }

    uint32_t ShaderPipelineMetaData::descriptorGroupsCount()
    {
        return _bindingGroups.size();
    }

    const DescriptorBindingGroup& ShaderPipelineMetaData::descriptorGroup(uint32_t index)
    {
        return _bindingGroups[index];
    }

    uint32_t ShaderPipelineMetaData::uniformBufferLayoutCount()
    {
        return _uniformBufferLayouts.size();
    }

    const BufferDescriptorBindingLayout& ShaderPipelineMetaData::uniformBufferLayout(uint32_t index)
    {
        return _uniformBufferLayouts[index];
    }

    uint32_t ShaderPipelineMetaData::storageBufferLayoutCount()
    {
        return _storageBufferLayouts.size();
    }

    const BufferDescriptorBindingLayout& ShaderPipelineMetaData::storageBufferLayout(uint32_t index)
    {
        return _storageBufferLayouts[index];
    }

    uint32_t ShaderPipelineMetaData::texelBufferDescriptionCount()
    {
        return _texelBufferDescriptions.size();
    }

    const TexelBufferDescriptorBinding& ShaderPipelineMetaData::texelBufferDescription(uint32_t index)
    {
        return _texelBufferDescriptions[index];
    }

    const BufferLayout& ShaderPipelineMetaData::pushConstantLayout()
    {
        return _pushConstantLayout;
    }

    uint32_t ShaderPipelineMetaData::xComputeThreads()
    {
        return _xComputeThreads;
    }

    uint32_t ShaderPipelineMetaData::yComputeThreads()
    {
        return _yComputeThreads;
    }

    uint32_t ShaderPipelineMetaData::zComputeThreads()
    {
        return _zComputeThreads;
    }

    struct PIPELINE_META_DATA_BoundDescriptorAndLayout
    {
        DescriptorBinding binding;
        BufferLayout layout;
        TexelBufferDescription bufferDescription;
    };

    void ShaderPipelineMetaData::build(ShaderCode** stagePointers, uint32_t stageCount)
    {
        std::vector<ShaderCode*> orderedStages(stageCount);
        memcpy(orderedStages.data(), stagePointers, sizeof(ShaderCode*) * stageCount);
        std::sort(orderedStages.begin(), orderedStages.end(),[&](ShaderCode* a, ShaderCode* b)
        {
            _stages |= a->stage();
            _stages |= b->stage();
            return ((uint16_t)a->stage()) < ((uint16_t)b->stage());
        });
        if (((uint16_t)(_stages & ShaderStageFlags::COMPUTE)) && stageCount !=1)
        {
            throw std::runtime_error("Cannot have a compute shader in a pipeline with any other stages");
        }
        if (((uint16_t)(_stages & (ShaderStageFlags::VERTEX | ShaderStageFlags::GEOMETRY))) && ((uint16_t)(_stages & (ShaderStageFlags::MESH | ShaderStageFlags::TASK))))
        {
            throw std::runtime_error("Cannot use both vertex pipeline stages and mesh pipeline stages together");
        }
        ShaderStageFlags lastStage = static_cast<ShaderStageFlags>(0);
        for (uint32_t i=0; i< stageCount; i++)
        {
            auto stage = orderedStages[i];
            if (stage->stage() == lastStage)
            {
                throw std::runtime_error("Multiple shaders have the same stage");
            }
            lastStage = stage->stage();
        }
        std::vector<std::unordered_map<uint64_t,PIPELINE_META_DATA_BoundDescriptorAndLayout>> combinedBindings;

        for (uint32_t i=0; i< stageCount; i++)
        {
            auto codeReflection = orderedStages[i]->metaData();
            if (codeReflection->pushConstantLayout().type() == GraphicsType::STRUCT)
            {
                if (_pushConstantLayout.type() == GraphicsType::UNKNOWN)
                {
                    _pushConstantLayout = codeReflection->pushConstantLayout();
                }
                else
                {
                    _pushConstantLayout = BufferLayout::merge(_pushConstantLayout,codeReflection->pushConstantLayout());
                }
            }
            if (codeReflection->stage() == ShaderStageFlags::COMPUTE)
            {
                _xComputeThreads = codeReflection->xComputeThreads();
                _yComputeThreads = codeReflection->yComputeThreads();
                _zComputeThreads = codeReflection->zComputeThreads();
            }
            if (codeReflection->stage() == ShaderStageFlags::VERTEX)
            {
                _vertexInputs.resize(codeReflection->inputCount());
                for (int j=0; j<codeReflection->inputCount(); j++)
                {
                    _vertexInputs[j] = codeReflection->inputVariable(j);
                }
            }
            for (int j=0; j< codeReflection->descriptorBindingGroupCount(); j++)
            {
                auto& descriptorGroup = codeReflection->descriptorBindingGroup(j);
                if (combinedBindings.size() <= descriptorGroup.descriptorGroupIndex())
                {
                    combinedBindings.resize(descriptorGroup.descriptorGroupIndex()+1);
                }
                auto& groupMap = combinedBindings[descriptorGroup.descriptorGroupIndex()];
                for (int descriptorBindingIndex=0; descriptorBindingIndex< descriptorGroup.bindingCount(); descriptorBindingIndex++)
                {
                    auto& binding = descriptorGroup.descriptorBinding(descriptorBindingIndex);
                    auto foundBinding = groupMap.find(binding.bindingId());
                    if (foundBinding == groupMap.end())
                    {
                        BufferLayout bindingLayout;
                        TexelBufferDescription texelDescription(Pixels::Format::UNDEFINED);

                        switch (binding.descriptor().shape().type)
                        {
                        case Descriptor::Type::UNIFORM_BUFFER:
                            bindingLayout = codeReflection->uniformBufferLayout(descriptorGroup.descriptorGroupIndex(),descriptorBindingIndex)->bufferLayout();
                            break;
                        case Descriptor::Type::STORAGE_BUFFER:
                            bindingLayout = codeReflection->storageBufferLayout(descriptorGroup.descriptorGroupIndex(),descriptorBindingIndex)->bufferLayout();
                            break;
                        case Descriptor::Type::UNIFORM_TEXEL_BUFFER:
                        case Descriptor::Type::STORAGE_TEXEL_BUFFER:
                            texelDescription = codeReflection->texelBufferDescription(descriptorGroup.descriptorGroupIndex(),descriptorBindingIndex)->bufferDescription();
                            break;
                        default:
                            break;
                        }
                        groupMap.insert(std::pair<uint64_t,PIPELINE_META_DATA_BoundDescriptorAndLayout>(binding.bindingId(),{binding,bindingLayout,texelDescription}));
                    }
                    else
                    {
                        auto& toUpdateBinding = foundBinding->second;
                        if (toUpdateBinding.binding.descriptor().shape() != binding.descriptor().shape())
                        {
                            throw std::runtime_error("Shader stages have incompatible descriptor groups");
                        }
                        switch (toUpdateBinding.binding.descriptor().shape().type)
                        {
                        case Descriptor::Type::UNIFORM_BUFFER:
                            toUpdateBinding.layout = BufferLayout::merge(toUpdateBinding.layout,codeReflection->uniformBufferLayout(descriptorGroup.descriptorGroupIndex(),descriptorBindingIndex)->bufferLayout());
                            break;
                        case Descriptor::Type::STORAGE_BUFFER:
                            toUpdateBinding.layout = BufferLayout::merge(toUpdateBinding.layout,codeReflection->storageBufferLayout(descriptorGroup.descriptorGroupIndex(),descriptorBindingIndex)->bufferLayout());
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }

        _bindingGroups.reserve(combinedBindings.size());

        for (int descriptorGroupIndex=0; descriptorGroupIndex< combinedBindings.size(); descriptorGroupIndex++)
        {
            auto& currentGroup = combinedBindings[descriptorGroupIndex];
            std::vector<DescriptorBinding> descriptors;
            descriptors.reserve(currentGroup.size());
            uint32_t currentDescriptorIndex = 0;
            for (auto& kvpair: currentGroup)
            {
                descriptors.push_back(kvpair.second.binding);
                auto type = kvpair.second.binding.descriptor().shape().type;
                if (type == Descriptor::Type::UNIFORM_BUFFER)
                {
                    auto layout = kvpair.second.layout;
                    _uniformBufferLayouts.push_back(BufferDescriptorBindingLayout(descriptorGroupIndex,currentDescriptorIndex,std::move(layout)));
                }
                else if (type == Descriptor::Type::STORAGE_BUFFER)
                {
                    auto layout = kvpair.second.layout;
                    _storageBufferLayouts.push_back(BufferDescriptorBindingLayout(descriptorGroupIndex,currentDescriptorIndex,std::move(layout)));
                }
                else if (type == Descriptor::Type::UNIFORM_TEXEL_BUFFER || type == Descriptor::Type::STORAGE_TEXEL_BUFFER)
                {
                    auto description = kvpair.second.bufferDescription;
                    _texelBufferDescriptions.push_back(TexelBufferDescriptorBinding(descriptorGroupIndex,currentDescriptorIndex,std::move(description)));
                }
                currentDescriptorIndex++;
            }
            _bindingGroups.push_back(DescriptorBindingGroup(descriptors.data(),descriptors.size(),descriptorGroupIndex));
        }
    }
} // slag