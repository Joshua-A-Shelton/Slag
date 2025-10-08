#include "DX12ShaderPipeline.h"

#include "DX12GraphicsCard.h"
#include "slag/backends/dx12/DX12Backend.h"
#include "slag/utilities/SLAG_ASSERT.h"
#include <slag/dxil_12/DXIL12Reflection.h>

namespace slag
{
    namespace dx12
    {
        DX12ShaderPipeline::DX12ShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties,VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)
        {
            SLAG_ASSERT(shaderCount > 0 && "Must have at least one shader provided for the pipeline");
            ShaderCode::CodeLanguage language = ShaderCode::CodeLanguage::CUSTOM;
            for (auto i=0; i<shaderCount; i++)
            {
                if (i ==0)
                {
                    language = shaders[i]->language();
                }
                else if (shaders[i]->language() != language)
                {
                    throw std::runtime_error("All shaders in a single pipeline must be the same language");
                }
            }
            if (language == ShaderCode::CodeLanguage::SPIRV)
            {
                spirvConstruct(shaders,shaderCount,properties,vertexDescription,framebufferDescription,rename,renameData);
            }
            else if (language == ShaderCode::CodeLanguage::DXIL)
            {
                dxilConstruct(shaders,shaderCount,properties,vertexDescription,framebufferDescription,rename,renameData);
            }
            else
            {
                throw std::runtime_error("Provided shaders are not in a format the current backend can use");
            }
        }

        DX12ShaderPipeline::DX12ShaderPipeline(const ShaderCode& computeShader,std::string(* rename)(const DescriptorRenameParameters&,void*), void* renameData)
        {
            throw std::runtime_error("DX12ShaderPipeline::DX12ShaderPipeline() not implemented");
        }

        DX12ShaderPipeline::~DX12ShaderPipeline()
        {
            if (_pipelineState)
            {
                _pipelineState->Release();
            }
        }

        DX12ShaderPipeline::DX12ShaderPipeline(DX12ShaderPipeline&& from)
        {
            move(from);
        }

        DX12ShaderPipeline& DX12ShaderPipeline::operator=(DX12ShaderPipeline&& from)
        {
            move(from);
            return *this;
        }

        ShaderPipeline::PipelineType DX12ShaderPipeline::pipelineType()
        {
            return _pipelineType;
        }

        uint32_t DX12ShaderPipeline::descriptorGroupCount()
        {
            return _descriptorGroups.size();
        }

        DescriptorGroup* DX12ShaderPipeline::descriptorGroup(uint32_t index)
        {
            return &_descriptorGroups.at(index);
        }

        DescriptorGroup* DX12ShaderPipeline::operator[](uint32_t index)
        {
            return &_descriptorGroups[index];
        }

        BufferLayout* DX12ShaderPipeline::bufferLayout(uint32_t descriptorGroup, uint32_t descriptorBinding)
        {
            auto group = _bufferLayouts.find(descriptorGroup);
            if(group == _bufferLayouts.end())
            {
                return nullptr;
            }
            auto description = group->second.find(descriptorBinding);
            if(description == group->second.end())
            {
                return nullptr;
            }
            return &description->second;
        }

        TexelBufferDescription* DX12ShaderPipeline::texelBufferDescription(uint32_t descriptorGroup,uint32_t descriptorBinding)
        {
            auto group = _texelBufferDescriptions.find(descriptorGroup);
            if(group == _texelBufferDescriptions.end())
            {
                return nullptr;
            }
            auto description = group->second.find(descriptorBinding);
            if(description == group->second.end())
            {
                return nullptr;
            }
            return &description->second;
        }

        uint32_t DX12ShaderPipeline::xComputeThreads()
        {
            return _xthreads;
        }

        uint32_t DX12ShaderPipeline::yComputeThreads()
        {
            return _ythreads;
        }

        uint32_t DX12ShaderPipeline::zComputeThreads()
        {
            return _zthreads;
        }

        void DX12ShaderPipeline::move(DX12ShaderPipeline& from)
        {
            std::swap(_pipelineState,from._pipelineState);
            PipelineType _pipelineType = from._pipelineType;
            _descriptorGroups.swap(from._descriptorGroups);
            _bufferLayouts.swap(from._bufferLayouts);
            _texelBufferDescriptions.swap(from._texelBufferDescriptions);
            _xthreads = from._xthreads;
            _ythreads = from._ythreads;
            _zthreads = from._zthreads;
        }

        void DX12ShaderPipeline::spirvConstruct(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription, std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)
        {
            throw std::runtime_error("SPIRV isn't supported yet for Direct X 12 Backend");
        }

        void DX12ShaderPipeline::dxilConstruct(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties,VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)
        {
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
            for (auto i=0; i<shaderCount; i++)
            {
                auto* code = shaders[i];
                switch (code->stage())
                {
                case ShaderStageFlags::VERTEX:
                    pipelineDesc.VS.pShaderBytecode = code->data();
                    pipelineDesc.VS.BytecodeLength = code->dataSize();
                    break;
                case ShaderStageFlags::FRAGMENT:
                    pipelineDesc.PS.pShaderBytecode = code->data();
                    pipelineDesc.PS.BytecodeLength = code->dataSize();
                    break;
                case ShaderStageFlags::GEOMETRY:
                    pipelineDesc.GS.pShaderBytecode = code->data();
                    pipelineDesc.GS.BytecodeLength = code->dataSize();
                    break;
                default:
                    throw std::runtime_error("Invalid shader stage provided for graphics pipeline");
                }
            }

            //TODO: Implement reflection data

            auto reflectionData = dxil::getReflectionData(shaders, shaderCount,rename,renameData);


            D3D12_GRAPHICS_PIPELINE_STATE_DESC shaderDescription{};
            shaderDescription.pRootSignature = reflectionData.rootSignature.Get();

            //shaderDescription.pRootSignature = REFLECTED_DATA_HERE;

            //shaderDescription.StreamOutput = ; //TODO: I don't think this is required to make a shader work, but may be required if I'm enabling streaming in the API....

            D3D12_BLEND_DESC& blendDesc = shaderDescription.BlendState;
            blendDesc.AlphaToCoverageEnable = !properties.multiSampleState.alphaToOneEnable; //FIXME: this is definitely a shot in the dark, no idea if it's correct
            blendDesc.IndependentBlendEnable = true;
            int colorTargetCount = 0;
            for(size_t i=0; i<8; i++)
            {
                if (framebufferDescription.colorTargets[i] == Pixels::Format::UNDEFINED)
                {
                    break;
                }
                colorTargetCount++;
                auto& target = blendDesc.RenderTarget[i];
                auto& value = properties.blendState.attachmentBlendStates[i];
                target.BlendEnable = value.blendingEnabled;
                target.LogicOpEnable = properties.blendState.logicOperationEnable;
                target.SrcBlend = DX12Backend::dx12blendFactor(value.srcColorBlendFactor);
                target.DestBlend = DX12Backend::dx12blendFactor(value.dstColorBlendFactor);
                target.BlendOp = DX12Backend::dx12BlendOp(value.colorBlendOperation);
                target.SrcBlendAlpha = DX12Backend::dx12blendFactor(value.srcAlphaBlendFactor);
                target.DestBlendAlpha = DX12Backend::dx12blendFactor(value.dstAlphaBlendFactor);
                target.BlendOpAlpha = DX12Backend::dx12BlendOp(value.alphaBlendOperation);
                target.LogicOp = DX12Backend::dx12LogicOp(properties.blendState.logicalOperation);
                if(static_cast<bool>(value.colorWriteMask & Color::ComponentFlags::RED_COMPONENT))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
                }
                if(static_cast<bool>(value.colorWriteMask & Color::ComponentFlags::GREEN_COMPONENT))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
                }
                if(static_cast<bool>(value.colorWriteMask & Color::ComponentFlags::BLUE_COMPONENT))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
                }
                if(static_cast<bool>(value.colorWriteMask & Color::ComponentFlags::ALPHA_COMPONENT))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
                }

            }

            shaderDescription.SampleMask = UINT_MAX;

            D3D12_RASTERIZER_DESC& rasterizerDesc = shaderDescription.RasterizerState;
            rasterizerDesc.FillMode = DX12Backend::dx12FillMode(properties.rasterizationState.drawMode);
            rasterizerDesc.CullMode = DX12Backend::dx12CullMode(properties.rasterizationState.culling);
            rasterizerDesc.FrontCounterClockwise = properties.rasterizationState.frontFacing == RasterizationState::FrontFacing::COUNTER_CLOCKWISE;
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
            depthStencilDesc.DepthFunc = DX12Backend::dx12ComparisonFunction(properties.depthStencilState.depthCompareOperation);
            depthStencilDesc.StencilEnable = properties.depthStencilState.stencilTestEnable;
            depthStencilDesc.StencilReadMask = properties.depthStencilState.stencilReadMask;
            depthStencilDesc.StencilWriteMask = properties.depthStencilState.stencilWriteMask;
            depthStencilDesc.FrontFace = DX12Backend::dx12StencilOpDesc(properties.depthStencilState.front);
            depthStencilDesc.BackFace = DX12Backend::dx12StencilOpDesc(properties.depthStencilState.back);

            D3D12_INPUT_LAYOUT_DESC& inputLayout = shaderDescription.InputLayout;
            std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

            for(auto i=0; i<vertexDescription.attributeChannels(); i++)
            {
                for(auto j=0; j< vertexDescription.attributeCount(i); j++)
                {

                    auto attr = vertexDescription.attribute(i,j);
                    auto formats = DX12Backend::dx12GraphicsType(attr.dataType());
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
                        description.AlignedByteOffset = attr.offset() + (k*DX12Backend::dx12FormatSize(description.Format));
                        description.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                        inputElements.push_back(description);
                    }
                }
            }


            inputLayout.pInputElementDescs = inputElements.data();
            inputLayout.NumElements = inputElements.size();

            shaderDescription.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//primitiveRestartEnable in vulkan
            shaderDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            shaderDescription.NumRenderTargets = colorTargetCount;

            for(auto i=0; i< colorTargetCount; i++)
            {
                shaderDescription.RTVFormats[i] = DX12Backend::dx12Format(framebufferDescription.colorTargets[i]);
            }

            shaderDescription.DSVFormat = DX12Backend::dx12Format(framebufferDescription.depthTarget);

            DXGI_SAMPLE_DESC& sampleDesc = shaderDescription.SampleDesc;
            sampleDesc.Count = properties.multiSampleState.rasterizationSamples;

            DX12GraphicsCard::selected()->device()->CreateGraphicsPipelineState(&shaderDescription, IID_PPV_ARGS(&_pipelineState));
        }
    } // dx12
} // slag
