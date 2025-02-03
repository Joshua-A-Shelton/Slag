#include "DX12ShaderPipeline.h"
#include "DX12Lib.h"
namespace slag
{
    namespace dx
    {
        DX12ShaderPipeline::DX12ShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC shaderDescription{};
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;
            for(int i=0; i< moduleCount; i++)
            {
                auto& module = modules[i];
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

            constructPipeline(properties, vertexDescription, frameBufferDescription, shaderDescription);

        }

        DX12ShaderPipeline::DX12ShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, const ShaderProperties& properties,VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC shaderDescription{};
            size_t vertexStageIndex = SIZE_MAX;
            size_t fragmentStageIndex = SIZE_MAX;
            for(int i=0; i< moduleCount; i++)
            {
                auto& module = *modules[i];
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

            constructPipeline(properties, vertexDescription, frameBufferDescription, shaderDescription);
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

        void DX12ShaderPipeline::constructPipeline(const ShaderProperties& properties, VertexDescription* vertexDescription, const FrameBufferDescription& frameBufferDescription,D3D12_GRAPHICS_PIPELINE_STATE_DESC& shaderDescription)
        {
            //TODO: get push constant and descriptor groups via reflection

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

                        for(auto k=0; k<formats.size(); i++)
                        {
                            D3D12_INPUT_ELEMENT_DESC description{};
                            description.SemanticName = semanticName.c_str();
                            description.SemanticIndex = k;
                            description.Format = formats[k];
                            description.InputSlot = i;//not too sure about this, but I think it's right
                            description.AlignedByteOffset = attr.offset() + (k*DX12Lib::formatSize(description.Format));
                            description.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                            inputElements.push_back(description);
                        }
                    }
                }
            }
            else
            {
                throw std::runtime_error("vertex description parsing not available for DX12 yet");
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

        PushConstantRange DX12ShaderPipeline::pushConstantRange(size_t index)
        {
            return _pushConstantRanges.at(index);
        }

    } // dx
} // slag