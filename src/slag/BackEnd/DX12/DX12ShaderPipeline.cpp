#include "DX12ShaderPipeline.h"
#include "DX12Lib.h"
namespace slag
{
    namespace dx
    {
        DX12ShaderPipeline::DX12ShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            std::vector<stageDetails> shaderStageData;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC shaderDescription{};
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;
            for(int i=0; i< moduleCount; i++)
            {
                auto& module = modules[i];
                shaderStageData.push_back(stageDetails(module));
                auto type = std::bit_cast<D3D12_SHADER_VERSION_TYPE>(module.stage());
                switch(type)
                {
                    case D3D12_SHVER_PIXEL_SHADER:
                        shaderDescription.PS.pShaderBytecode = module.data();
                        shaderDescription.PS.BytecodeLength = module.dataSize();
                        fragmentStageIndex = i;
                        break;
                    case D3D12_SHVER_VERTEX_SHADER:
                        shaderDescription.VS.pShaderBytecode = module.data();
                        shaderDescription.VS.BytecodeLength = module.dataSize();
                        vertexStageIndex = i;
                        break;
                    case D3D12_SHVER_GEOMETRY_SHADER:
                        shaderDescription.GS.pShaderBytecode = module.data();
                        shaderDescription.GS.BytecodeLength = module.dataSize();
                        break;
                    case D3D12_SHVER_HULL_SHADER:
                        shaderDescription.HS.pShaderBytecode = module.data();
                        shaderDescription.HS.BytecodeLength = module.dataSize();
                        break;
                    case D3D12_SHVER_DOMAIN_SHADER:
                        shaderDescription.DS.pShaderBytecode = module.data();
                        shaderDescription.DS.BytecodeLength = module.dataSize();
                        break;
                    default:
                        throw std::runtime_error("Stage not permitted for graphics shader at index");
                        break;
                }
            }
            if(vertexStageIndex==SIZE_MAX || fragmentStageIndex==SIZE_MAX)
            {
                throw std::runtime_error("Must define both a vertex stage and fragment stage");
            }

            constructPipeline(descriptorGroups,descriptorGroupCount,properties, vertexDescription, frameBufferDescription, shaderDescription,shaderStageData,vertexStageIndex);

        }

        DX12ShaderPipeline::DX12ShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties,VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            std::vector<stageDetails> shaderStageData;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC shaderDescription{};
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;
            for(int i=0; i< moduleCount; i++)
            {
                auto& module = *modules[i];
                shaderStageData.push_back(stageDetails(module));
                auto type = std::bit_cast<D3D12_SHADER_VERSION_TYPE>(module.stage());
                switch(type)
                {
                    case D3D12_SHVER_PIXEL_SHADER:
                        shaderDescription.PS.pShaderBytecode = module.data();
                        shaderDescription.PS.BytecodeLength = module.dataSize();
                        fragmentStageIndex = i;
                        break;
                    case D3D12_SHVER_VERTEX_SHADER:
                        shaderDescription.VS.pShaderBytecode = module.data();
                        shaderDescription.VS.BytecodeLength = module.dataSize();
                        vertexStageIndex = i;
                        break;
                    case D3D12_SHVER_GEOMETRY_SHADER:
                        shaderDescription.GS.pShaderBytecode = module.data();
                        shaderDescription.GS.BytecodeLength = module.dataSize();
                        break;
                    case D3D12_SHVER_HULL_SHADER:
                        shaderDescription.HS.pShaderBytecode = module.data();
                        shaderDescription.HS.BytecodeLength = module.dataSize();
                        break;
                    case D3D12_SHVER_DOMAIN_SHADER:
                        shaderDescription.DS.pShaderBytecode = module.data();
                        shaderDescription.DS.BytecodeLength = module.dataSize();
                        break;
                    default:
                        throw std::runtime_error("Stage not permitted for graphics shader at index");
                        break;
                }
            }
            if(vertexStageIndex==SIZE_MAX || fragmentStageIndex==SIZE_MAX)
            {
                throw std::runtime_error("Must define both a vertex stage and fragment stage");
            }

            constructPipeline(descriptorGroups,descriptorGroupCount,properties, vertexDescription, frameBufferDescription, shaderDescription,shaderStageData,vertexStageIndex);
        }

        DX12ShaderPipeline::~DX12ShaderPipeline()
        {
            if(_pipeline)
            {
                smartDestroy();
            }
        }

        void DX12ShaderPipeline::move(DX12ShaderPipeline&& from)
        {
            std::swap(_pipeline,from._pipeline);
            _descriptorGroups.swap(from._descriptorGroups);
            _pushConstantRanges.swap(from._pushConstantRanges);
        }

        void DX12ShaderPipeline::constructPipeline(DescriptorGroup** descriptorGroups, const size_t descriptorGroupCount,const ShaderProperties& properties, VertexDescription* vertexDescription, const FrameBufferDescription& frameBufferDescription,D3D12_GRAPHICS_PIPELINE_STATE_DESC& shaderDescription,std::vector<stageDetails>& shaderStageData, size_t vertexStageIndex)
        {
            //get push constant and descriptor groups via reflection
            std::unordered_map<size_t,std::vector<DX12DescriptorGroup>> reflectedDescriptorGroups;
            size_t maxDescriptorGroup = descriptorGroupCount;
            bool hasDescriptorGroups = false;
            for(size_t i=0; i< shaderStageData.size(); i++)
            {
                auto& reflectModule = shaderStageData[i].reflectModule;
                auto stageFlags = shaderStageData[i].stageFlags;
                uint32_t setCount = 0;
                auto result = spvReflectEnumerateDescriptorSets(&reflectModule,&setCount, nullptr);
                if(setCount > 0)
                {
                    hasDescriptorGroups = true;
                }
                for(size_t set = 0; set< setCount; set++)
                {
                    auto binding = reflectModule.descriptor_bindings[set];
                    //if we've passed an override, don't bother figuring all this out
                    if(binding.set < descriptorGroupCount)
                    {
                        continue;
                    }
                    else if(binding.set > maxDescriptorGroup)
                    {
                        maxDescriptorGroup = binding.set;
                    }
                    auto descriptorSet = spvReflectGetDescriptorSet(&reflectModule,binding.set,&result);
                    if(descriptorSet != nullptr)
                    {
                        std::vector<Descriptor> setDescriptors;
                        for(uint32_t descriptorIndex=0; descriptorIndex<descriptorSet->binding_count; descriptorIndex++)
                        {
                            auto desc = descriptorSet->bindings[descriptorIndex];
                            setDescriptors.push_back(Descriptor(desc->name,lib::BackEndLib::descriptorTypeFromSPV(desc->descriptor_type),desc->count,desc->binding,stageFlags));
                        }
                        if(reflectedDescriptorGroups.contains(binding.set))
                        {
                            reflectedDescriptorGroups[binding.set] = std::vector<DX12DescriptorGroup>();
                        }
                        auto& group = reflectedDescriptorGroups[binding.set];
                        group.push_back(DX12DescriptorGroup(setDescriptors.data(),setDescriptors.size()));
                    }
                }
                uint32_t blockCount = 0;
                result = spvReflectEnumeratePushConstantBlocks(&reflectModule,&blockCount, nullptr);
                for(uint32_t blockIndex=0; blockIndex<blockCount; blockIndex++)
                {
                    auto& range = *spvReflectGetPushConstantBlock(&reflectModule,blockIndex,&result);
                    _pushConstantRanges.push_back(PushConstantRange{.stageFlags = stageFlags,.offset = range.offset,.size = range.size});
                    //TODO: acquire actual variables and assign them to the ranges
                }
            }

            //add descriptor groups to shader
            for(size_t i=0; i<=maxDescriptorGroup && hasDescriptorGroups; i++)
            {
                //if we provided an override for a group, use that
                if(i < descriptorGroupCount)
                {
                    _descriptorGroups.push_back(*static_cast<DX12DescriptorGroup*>(descriptorGroups[i]));
                }
                    //otherwise, merge the groups we found from reflection and use that
                else
                {
                    auto& groups = reflectedDescriptorGroups[i];
                    std::vector<DescriptorGroup*> groupPointers(groups.size());
                    for(size_t j=0; j<groups.size(); j++)
                    {
                        groupPointers[j] = &groups[j];
                    }
                    auto descriptors = DescriptorGroup::combine(groupPointers.data(),groupPointers.size());
                    _descriptorGroups.push_back(DX12DescriptorGroup(descriptors.data(), descriptors.size()));
                }
            }










            //shaderDescription.StreamOutput = ; //TODO: I don't think this is required to make a shader work, but may be required if I'm enabling streaming in the API....

            D3D12_BLEND_DESC& blendDesc = shaderDescription.BlendState;
            blendDesc.AlphaToCoverageEnable = !properties.multiSampleState.alphaToOneEnable; //FIXME: this is definitely a shot in the dark, no idea if it's correct
            blendDesc.IndependentBlendEnable = true;
            for(size_t i=0; i< frameBufferDescription.colorTargetCount() &&  i<8; i++)
            {
                auto& target = blendDesc.RenderTarget[i];
                auto& value = properties.blendState.attachmentBlendStates[i];
                target.BlendEnable = value.blendingEnabled;
                target.LogicOpEnable = properties.blendState.logicOperationEnable;
                target.SrcBlend = DX12Lib::blendFactor(value.srcColorBlendFactor);
                target.DestBlend = DX12Lib::blendFactor(value.dstColorBlendFactor);
                target.BlendOp = DX12Lib::blendOperation(value.colorBlendOperation);
                target.SrcBlendAlpha = DX12Lib::blendFactor(value.srcAlphaBlendFactor);
                target.DestBlendAlpha = DX12Lib::blendFactor(value.dstAlphaBlendFactor);
                target.BlendOpAlpha = DX12Lib::blendOperation(value.alphaBlendOperation);
                target.LogicOp = DX12Lib::logicalOperation(properties.blendState.logicalOperation);
                if(value.colorWriteMask & Color::RED_COMPONENT)
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
                }
                if(value.colorWriteMask & Color::GREEN_COMPONENT)
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
                }
                if(value.colorWriteMask & Color::BLUE_COMPONENT)
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
                }
                if(value.colorWriteMask & Color::ALPHA_COMPONENT)
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
                }

            }

            shaderDescription.SampleMask = UINT_MAX;

            D3D12_RASTERIZER_DESC& rasterizerDesc = shaderDescription.RasterizerState;
            rasterizerDesc.FillMode = DX12Lib::fillMode(properties.rasterizationState.drawMode);
            rasterizerDesc.CullMode = DX12Lib::cullMode(properties.rasterizationState.culling);
            rasterizerDesc.FrontCounterClockwise = properties.rasterizationState.frontFacing == RasterizationState::COUNTER_CLOCKWISE;
            rasterizerDesc.DepthBias = properties.rasterizationState.depthBiasEnable? properties.rasterizationState.depthBiasConstantFactor : 0;
            rasterizerDesc.DepthBiasClamp = properties.rasterizationState.depthBiasEnable? properties.rasterizationState.depthBiasClamp : 0;
            rasterizerDesc.SlopeScaledDepthBias = properties.rasterizationState.depthBiasEnable? properties.rasterizationState.depthBiasSlopeFactor : 0;
            rasterizerDesc.DepthClipEnable = properties.rasterizationState.depthClampEnable;//TODO: not sure if this is the corresponding property, but seems likely
            rasterizerDesc.MultisampleEnable = properties.multiSampleState.sampleShadingEnable;
            rasterizerDesc.AntialiasedLineEnable = true;//Seems sensible default, only applies if doing line rendering and no multisampling
            rasterizerDesc.ForcedSampleCount = properties.multiSampleState.minSampleShading;
            rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;//Eh? nothing like this in vulkan

            D3D12_DEPTH_STENCIL_DESC& depthStencilDesc = shaderDescription.DepthStencilState;
            depthStencilDesc.DepthEnable = properties.depthStencilState.depthTestEnable;
            depthStencilDesc.DepthWriteMask = properties.depthStencilState.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            depthStencilDesc.DepthFunc = DX12Lib::comparisonFunction(properties.depthStencilState.depthCompareOperation);
            depthStencilDesc.StencilEnable = properties.depthStencilState.stencilTestEnable;
            depthStencilDesc.StencilReadMask = properties.depthStencilState.stencilReadMask;
            depthStencilDesc.StencilWriteMask = properties.depthStencilState.stencilWriteMask;
            depthStencilDesc.FrontFace = DX12Lib::stencilopState(properties.depthStencilState.front);
            depthStencilDesc.BackFace = DX12Lib::stencilopState(properties.depthStencilState.back);

            D3D12_INPUT_LAYOUT_DESC& inputLayout = shaderDescription.InputLayout;
            std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
            if(vertexDescription)
            {
                for(auto i=0; i<vertexDescription->attributeChannels(); i++)
                {
                    for(auto j=0; j< vertexDescription->attributeCount(i); j++)
                    {

                        auto attr = vertexDescription->attribute(i,j);
                        auto formats = DX12Lib::graphicsType(attr.dataType());
                        if(formats.empty())
                        {
                            throw std::runtime_error("Unable to convert graphicsType type into underlying API type");
                        }
                        std::string semanticName("element");
                        semanticName+=std::to_string(i);
                        semanticName+="_";
                        semanticName+=std::to_string(j);

                        for(auto k=0; k<formats.size(); k++)
                        {
                            D3D12_INPUT_ELEMENT_DESC description{};
                            description.SemanticName = semanticName.c_str();
                            description.SemanticIndex = i;
                            description.Format = formats[k];
                            description.InputSlot = j;//not too sure about this, but I think it's right
                            description.AlignedByteOffset = attr.offset() + (k*DX12Lib::formatSize(description.Format));
                            description.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                            inputElements.push_back(description);
                        }
                    }
                }
            }
            else
            {
                auto& vertexModule = shaderStageData[vertexStageIndex].reflectModule;
                uint32_t offset = 0;
                for(uint32_t i=0; i< vertexModule.input_variable_count; i++)
                {
                    auto inputVar = vertexModule.input_variables[i];
                    if(inputVar->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                    {
                        continue;
                    }
                    auto type = lib::BackEndLib::graphicsTypeFromSPV(inputVar->format);
                    if(type == GraphicsTypes::UNKNOWN)
                    {
                        throw std::runtime_error("Vertex Shader Module contains vertex attribute of unknown type");
                    }

                    auto formats = DX12Lib::graphicsType(type);
                    if(formats.empty())
                    {
                        throw std::runtime_error("Unable to convert graphicsType type into underlying API type");
                    }
                    std::string semanticName("element");
                    semanticName+="0_";
                    semanticName+=std::to_string(i);

                    for(auto k=0; k<formats.size(); k++)
                    {
                        D3D12_INPUT_ELEMENT_DESC description{};
                        description.SemanticName = semanticName.c_str();
                        description.SemanticIndex = i;
                        description.Format = formats[k];
                        description.InputSlot = 0;//not too sure about this, but I think it's right
                        description.AlignedByteOffset = offset;
                        description.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                        inputElements.push_back(description);

                        offset+= DX12Lib::formatSize(formats[k]);
                    }
                }

            }

            inputLayout.pInputElementDescs = inputElements.data();
            inputLayout.NumElements = inputElements.size();

            shaderDescription.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//primitiveRestartEnable in vulkan
            shaderDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            shaderDescription.NumRenderTargets = frameBufferDescription.colorTargetCount();

            for(auto i=0; i< frameBufferDescription.colorTargetCount(); i++)
            {
                shaderDescription.RTVFormats[i] = DX12Lib::format(frameBufferDescription.colorFormat(i));
            }

            shaderDescription.DSVFormat = DX12Lib::format(frameBufferDescription.depthFormat());

            DXGI_SAMPLE_DESC& sampleDesc = shaderDescription.SampleDesc;
            sampleDesc.Count = properties.multiSampleState.rasterizationSamples;

            DX12Lib::card()->device()->CreateGraphicsPipelineState(&shaderDescription, IID_PPV_ARGS(&_pipeline));
            auto pline = _pipeline;
            _disposeFunction = [=]()
            {
                pline->Release();
            };
        }

        size_t DX12ShaderPipeline::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* DX12ShaderPipeline::descriptorGroup(size_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* DX12ShaderPipeline::operator[](size_t index)
        {
            return &_descriptorGroups[index];
        }

        size_t DX12ShaderPipeline::pushConstantRangeCount()
        {
            return _pushConstantRanges.size();
        }

        UniformBufferDescriptorLayout* DX12ShaderPipeline::uniformBufferLayout(size_t descriptorGroup,uint32_t descriptorBinding)
        {
            auto it = _uniformBufferLayouts.find(descriptorGroup);
            if(it == _uniformBufferLayouts.end())
            {
                return nullptr;
            }
            auto& layouts = it->second;
            auto it2 = layouts.find(descriptorBinding);
            if(it2 == layouts.end())
            {
                return nullptr;
            }
            return &layouts.at(descriptorBinding);
        }

        PushConstantRange DX12ShaderPipeline::pushConstantRange(size_t index)
        {
            return _pushConstantRanges.at(index);
        }

    } // dx
} // slag