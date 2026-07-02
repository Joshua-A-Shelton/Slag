#include "DX12ShaderPipeline.h"

#include "DX12Backend.h"
#include "DX12GraphicsCard.h"
#include "directx/d3dx12_core.h"
#include "directx/d3dx12_default.h"
#include "slag/exceptions/InvalidShaderCodeError.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12ShaderPipeline::DX12ShaderPipeline(
            DX12GraphicsCard* graphicsCard,
            const VertexDescription& vertexDescription,
            const ShaderCode& vertexShader,
            const ShaderCode& fragmentShader,
            const PipelineState& pipelineState,
            const FramebufferDescription& framebufferDescription)
        {
            _graphicsCard = graphicsCard;
            _type = ShaderPipelineType::GRAPHICS;

            D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};
            pipelineDesc.VS.pShaderBytecode = vertexShader.code;
            pipelineDesc.VS.BytecodeLength = vertexShader.codeLength;

            pipelineDesc.PS.pShaderBytecode = fragmentShader.code;
            pipelineDesc.PS.BytecodeLength = fragmentShader.codeLength;

            pipelineDesc.pRootSignature = _graphicsCard->rootSignature();

            D3D12_BLEND_DESC& blendDesc = pipelineDesc.BlendState;
            blendDesc.AlphaToCoverageEnable = !pipelineState.multiSampleState.alphaToOneEnable; //FIXME: this is definitely a shot in the dark, no idea if it's correct
            blendDesc.IndependentBlendEnable = true;
            int colorTargetCount = 0;
            for(size_t i=0; i<8; i++)
            {
                if (framebufferDescription.colorFormats[i] == PixelFormat::UNDEFINED)
                {
                    break;
                }
                colorTargetCount++;
                auto& target = blendDesc.RenderTarget[i];
                auto& value = pipelineState.blendState.attachmentBlendStates[i];
                target.BlendEnable = value.blendingEnabled;
                target.LogicOpEnable = pipelineState.blendState.logicOperationEnable;
                target.SrcBlend = DX12Backend::nativeBlendFactor(value.srcColorBlendFactor);
                target.DestBlend = DX12Backend::nativeBlendFactor(value.dstColorBlendFactor);
                target.BlendOp = DX12Backend::nativeBlendOp(value.colorBlendOperation);
                target.SrcBlendAlpha = DX12Backend::nativeBlendFactor(value.srcAlphaBlendFactor);
                target.DestBlendAlpha = DX12Backend::nativeBlendFactor(value.dstAlphaBlendFactor);
                target.BlendOpAlpha = DX12Backend::nativeBlendOp(value.alphaBlendOperation);
                target.LogicOp = DX12Backend::nativeLogicOp(pipelineState.blendState.logicalOperation);
                if(static_cast<bool>(value.colorWriteMask & ColorComponents::RED))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_RED;
                }
                if(static_cast<bool>(value.colorWriteMask & ColorComponents::GREEN))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
                }
                if(static_cast<bool>(value.colorWriteMask & ColorComponents::BLUE))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
                }
                if(static_cast<bool>(value.colorWriteMask & ColorComponents::ALPHA))
                {
                    target.RenderTargetWriteMask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
                }

            }

            pipelineDesc.SampleMask = UINT_MAX;

            D3D12_RASTERIZER_DESC& rasterizerDesc = pipelineDesc.RasterizerState;
            rasterizerDesc.FillMode = DX12Backend::nativeFillMode(pipelineState.rasterizationState.drawMode);
            rasterizerDesc.CullMode = DX12Backend::nativeCullMode(pipelineState.rasterizationState.culling);
            rasterizerDesc.FrontCounterClockwise = pipelineState.rasterizationState.frontFacing == RasterizationState::FrontFacing::COUNTER_CLOCKWISE;
            rasterizerDesc.DepthBias = pipelineState.rasterizationState.depthBiasEnable? pipelineState.rasterizationState.depthBiasConstantFactor : 0;
            rasterizerDesc.DepthBiasClamp = pipelineState.rasterizationState.depthBiasEnable? pipelineState.rasterizationState.depthBiasClamp : 0;
            rasterizerDesc.SlopeScaledDepthBias = pipelineState.rasterizationState.depthBiasEnable? pipelineState.rasterizationState.depthBiasSlopeFactor : 0;
            rasterizerDesc.DepthClipEnable = pipelineState.rasterizationState.depthClampEnable;//TODO: not sure if this is the corresponding property, but seems likely
            rasterizerDesc.MultisampleEnable = pipelineState.multiSampleState.sampleShadingEnable;
            rasterizerDesc.AntialiasedLineEnable = true;//Seems sensible default, only applies if doing line rendering and no multisampling
            if (pipelineState.multiSampleState.sampleShadingEnable)
            {
                //TODO: not sure what this actually is, but setting it causes errors (currently hidden by D3D12_MESSAGE_CATEGORY_STATE_CREATION being a hidden category in graphics card creation)
                rasterizerDesc.ForcedSampleCount = pipelineState.multiSampleState.minSampleShading;
            }

            rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;//Eh? nothing like this in vulkan

            D3D12_DEPTH_STENCIL_DESC& depthStencilDesc = pipelineDesc.DepthStencilState;
            depthStencilDesc.DepthEnable = pipelineState.depthStencilState.depthTestEnable;
            depthStencilDesc.DepthWriteMask = pipelineState.depthStencilState.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            depthStencilDesc.DepthFunc = DX12Backend::nativeCompareFunc(pipelineState.depthStencilState.depthCompareOperation);
            depthStencilDesc.StencilEnable = pipelineState.depthStencilState.stencilTestEnable;
            depthStencilDesc.StencilReadMask = pipelineState.depthStencilState.stencilReadMask;
            depthStencilDesc.StencilWriteMask = pipelineState.depthStencilState.stencilWriteMask;
            depthStencilDesc.FrontFace = DX12Backend::nativeDepthStencilOpDesc(pipelineState.depthStencilState.front);
            depthStencilDesc.BackFace = DX12Backend::nativeDepthStencilOpDesc(pipelineState.depthStencilState.back);

            D3D12_INPUT_LAYOUT_DESC& inputLayout = pipelineDesc.InputLayout;
            std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;

            for (auto i=0; i<vertexDescription.bindingCount(); i++)
            {
                auto& binding = vertexDescription[i];
                for (auto j=0; j< binding.attributeCount(); j++)
                {
                    auto& attr = binding[j];

                    auto stepRate = binding.inputRate() == InputRate::PER_INSTANCE ? 1u : 0u;

                    inputElements.push_back(D3D12_INPUT_ELEMENT_DESC{
                        .SemanticName = attr.name().c_str(),
                        .SemanticIndex = 0,
                        .Format = DX12Backend::nativeFormat(attr.loadAs()),
                        .InputSlot = binding.bindingIndex(),
                        .AlignedByteOffset = attr.offset(),
                        .InputSlotClass = DX12Backend::nativeVertexInputRate(binding.inputRate()),
                        .InstanceDataStepRate = stepRate,
                    });
                }
            }


            inputLayout.pInputElementDescs = inputElements.data();
            inputLayout.NumElements = inputElements.size();

            pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//primitiveRestartEnable in vulkan
            pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            pipelineDesc.NumRenderTargets = colorTargetCount;

            for(auto i=0; i< colorTargetCount; i++)
            {
                pipelineDesc.RTVFormats[i] = DX12Backend::nativeFormat(framebufferDescription.colorFormats[i]);
            }

            pipelineDesc.DSVFormat = DX12Backend::nativeFormat(framebufferDescription.depthFormat);

            DXGI_SAMPLE_DESC& sampleDesc = pipelineDesc.SampleDesc;
            sampleDesc.Count = pipelineState.multiSampleState.rasterizationSamples;

            auto result = _graphicsCard->device()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&_pipelineState));
            if (FAILED(result))
            {
                throw InvalidShaderCodeError("Unable to create shader pipeline");
            }

        }

        DX12ShaderPipeline::DX12ShaderPipeline(DX12ShaderPipeline&& from) noexcept
        {
            move(from);
        }

        DX12ShaderPipeline& DX12ShaderPipeline::operator=(DX12ShaderPipeline&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12ShaderPipeline::~DX12ShaderPipeline()
        {
            if (_pipelineState)
            {
                _pipelineState->Release();
            }
        }

        ShaderPipelineType DX12ShaderPipeline::type()
        {
            return _type;
        }

        GraphicsCard* DX12ShaderPipeline::graphicsCard()
        {
            return _graphicsCard;
        }

        ID3D12PipelineState* DX12ShaderPipeline::dx12Handle() const
        {
            return _pipelineState;
        }

        void DX12ShaderPipeline::move(DX12ShaderPipeline& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_pipelineState,from._pipelineState);
            _type = from._type;
        }
    } // dx12
} // slag