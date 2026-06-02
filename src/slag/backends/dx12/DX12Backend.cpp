#include "DX12Backend.h"

#include "slag/exceptions/NotImplemented.h"

namespace slag
{
    namespace dx12
    {
        DXGI_FORMAT DX12_FORMATS[]
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_D32_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_D16_UNORM,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8_SNORM,
            DXGI_FORMAT_R8_SINT,
            DXGI_FORMAT_A8_UNORM,
            DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
            DXGI_FORMAT_R8G8_B8G8_UNORM,
            DXGI_FORMAT_G8R8_G8B8_UNORM,
            DXGI_FORMAT_BC1_UNORM,
            DXGI_FORMAT_BC1_UNORM_SRGB,
            DXGI_FORMAT_BC2_UNORM,
            DXGI_FORMAT_BC2_UNORM_SRGB,
            DXGI_FORMAT_BC3_UNORM,
            DXGI_FORMAT_BC3_UNORM_SRGB,
            DXGI_FORMAT_BC4_UNORM,
            DXGI_FORMAT_BC4_SNORM,
            DXGI_FORMAT_BC5_UNORM,
            DXGI_FORMAT_BC5_SNORM,
            DXGI_FORMAT_B5G6R5_UNORM,
            DXGI_FORMAT_B5G5R5A1_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_B8G8R8X8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
            DXGI_FORMAT_BC6H_UF16,
            DXGI_FORMAT_BC6H_SF16,
            DXGI_FORMAT_BC7_UNORM,
            DXGI_FORMAT_BC7_UNORM_SRGB,
            DXGI_FORMAT_AYUV,
            DXGI_FORMAT_NV12,
            DXGI_FORMAT_420_OPAQUE,
            DXGI_FORMAT_YUY2,
            DXGI_FORMAT_B4G4R4A4_UNORM
        };


        DX12Backend::DX12Backend()
        {
        }

        BackendAPI DX12Backend::api()const
        {
            return BackendAPI::DX12;
        }

        uint32_t DX12Backend::graphicsCardCount()const
        {
            return _graphicsCards.size();
        }

        GraphicsCard* DX12Backend::graphicsCard(uint32_t index)
        {
            return &_graphicsCards[index];
        }

        uint32_t DX12Backend::supportedShaderLanguageCount() const
        {
            return 1;
        }

        ShaderLanguage DX12Backend::supportedShaderLanguage(uint32_t index) const
        {
            return ShaderLanguage::DXIL;
        }

        SlagDebugHandlerPtr DX12Backend::getDebugHandler() const
        {
            return _debugHandler;
        }

        DXGI_FORMAT DX12Backend::nativeFormat(PixelFormat format)
        {
            return DX12_FORMATS[(uint32_t)format];
        }

        D3D12_RESOURCE_FLAGS DX12Backend::nativeTextureUsageFlags(TextureUsageFlags usage)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if(static_cast<uint8_t>(usage & TextureUsageFlags::DEPTH_STENCIL_TARGET))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            if(static_cast<uint8_t>(usage & TextureUsageFlags::COLOR_TARGET))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if (static_cast<uint8_t>(usage & TextureUsageFlags::UNORDERED_ACCESS))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            return flags;
        }

        D3D12_BARRIER_ACCESS DX12Backend::nativeMemoryCaches(MemoryCaches caches)
        {

            D3D12_BARRIER_ACCESS flags = D3D12_BARRIER_ACCESS_COMMON;

            if (static_cast<bool>(caches & MemoryCaches::INDIRECT_COMMAND_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
            }
            if (static_cast<bool>(caches & MemoryCaches::INDEX_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
            }
            if (static_cast<bool>(caches & MemoryCaches::VERTEX_ATTRIBUTE_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
            }
            if (static_cast<bool>(caches & MemoryCaches::UNIFORM_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
            }
            if (static_cast<bool>(caches & MemoryCaches::COLOR_TARGET))
            {
                flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
            }
            if (static_cast<bool>(caches & MemoryCaches::DEPTH_TARGET_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
            }
            if (static_cast<bool>(caches & MemoryCaches::DEPTH_TARGET_WRITE))
            {
                flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
            }
            if (static_cast<bool>(caches & MemoryCaches::SHADER_SAMPLED_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
            }
            if (static_cast<bool>(caches & MemoryCaches::SHADER_UNORDERED_ACCESS))
            {
                flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            }
            if (static_cast<bool>(caches & MemoryCaches::BLIT_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            }
            if (static_cast<bool>(caches & MemoryCaches::BLIT_WRITE))
            {
                flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            }
            if (static_cast<bool>(caches & MemoryCaches::COPY_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
            }
            if (static_cast<bool>(caches & MemoryCaches::COPY_WRITE))
            {
                flags |= D3D12_BARRIER_ACCESS_COPY_DEST;
            }
            if (static_cast<bool>(caches & MemoryCaches::RESOLVE_READ))
            {
                flags |= D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;
            }
            if (static_cast<bool>(caches & MemoryCaches::RESOLVE_WRITE))
            {
                flags |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;
            }
            if (static_cast<bool>(caches & MemoryCaches::CLEAR))
            {
                flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
            }
            return flags;
        }

        D3D12_BARRIER_SYNC DX12Backend::nativePipelineStages(SyncStages stages)
        {
            D3D12_BARRIER_SYNC flags = D3D12_BARRIER_SYNC_NONE;

            if (static_cast<bool>(stages & SyncStages::ALL))
            {
                flags |= D3D12_BARRIER_SYNC_ALL;
            }
            if (static_cast<bool>(stages & SyncStages::ALL_GRAPHICS))
            {
                flags |= D3D12_BARRIER_SYNC_DRAW;
            }
            if (static_cast<bool>(stages & SyncStages::INDEX_INPUT))
            {
                flags |= D3D12_BARRIER_SYNC_INDEX_INPUT;
            }
            if (static_cast<bool>(stages & SyncStages::VERTEX_SHADER))
            {
                flags |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
            }
            if (static_cast<bool>(stages & SyncStages::FRAGMENT_SHADER))
            {
                flags |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
            }
            if (static_cast<bool>(stages & SyncStages::DEPTH_STENCIL_TARGET_OUTPUT))
            {
                flags |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
            }
            if (static_cast<bool>(stages & SyncStages::COLOR_TARGET_OUTPUT))
            {
                flags |= D3D12_BARRIER_SYNC_RENDER_TARGET;
            }
            if (static_cast<bool>(stages & SyncStages::COMPUTE_SHADER))
            {
                flags |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
            }
            if (static_cast<bool>(stages & SyncStages::RAYTRACING_SHADER))
            {
                flags |= D3D12_BARRIER_SYNC_RAYTRACING;
            }
            if (static_cast<bool>(stages & SyncStages::COPY))
            {
                flags |= D3D12_BARRIER_SYNC_COPY;
            }
            if (static_cast<bool>(stages & SyncStages::RESOLVE))
            {
                flags |= D3D12_BARRIER_SYNC_RESOLVE;
            }
            if (static_cast<bool>(stages & SyncStages::EXECUTE_INDIRECT))
            {
                flags |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
            }
            if (static_cast<bool>(stages & SyncStages::CLEAR))
            {
                flags |= D3D12_BARRIER_SYNC_RENDER_TARGET | D3D12_BARRIER_SYNC_DEPTH_STENCIL;
            }
            if (static_cast<bool>(stages & SyncStages::VIDEO_DECODE))
            {
                flags |= D3D12_BARRIER_SYNC_VIDEO_DECODE;
            }
            if (static_cast<bool>(stages & SyncStages::VIDEO_ENCODE))
            {
                flags |= D3D12_BARRIER_SYNC_VIDEO_ENCODE;
            }
            if (static_cast<bool>(stages & SyncStages::BUILD_ACCELERATION_STRUCTURE))
            {
                flags |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
            }
            if (static_cast<bool>(stages & SyncStages::COPY_ACCELERATION_STRUCTURE))
            {
                flags |= D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE;
            }
            return flags;
        }

        D3D12_BLEND D3D12_NATIVE_BLEND_FACTORS[]
        {
            D3D12_BLEND_ZERO,
            D3D12_BLEND_ONE,
            D3D12_BLEND_SRC_COLOR,
            D3D12_BLEND_INV_SRC_COLOR,
            D3D12_BLEND_DEST_COLOR,
            D3D12_BLEND_INV_DEST_COLOR,
            D3D12_BLEND_SRC_ALPHA,
            D3D12_BLEND_INV_SRC_ALPHA,
            D3D12_BLEND_DEST_ALPHA,
            D3D12_BLEND_INV_DEST_ALPHA,
            D3D12_BLEND_BLEND_FACTOR,
            D3D12_BLEND_INV_BLEND_FACTOR,
            D3D12_BLEND_ALPHA_FACTOR,
            D3D12_BLEND_INV_ALPHA_FACTOR,
            D3D12_BLEND_SRC_ALPHA_SAT,
            D3D12_BLEND_SRC1_COLOR,
            D3D12_BLEND_INV_SRC1_COLOR,
            D3D12_BLEND_SRC1_ALPHA,
            D3D12_BLEND_INV_SRC1_ALPHA
        };
        D3D12_BLEND DX12Backend::nativeBlendFactor(BlendFactor factor)
        {
            return D3D12_NATIVE_BLEND_FACTORS[(uint32_t)factor];
        }


        D3D12_BLEND_OP D3D12_NATIVE_BLEND_OPS[]
        {
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_OP_SUBTRACT,
            D3D12_BLEND_OP_REV_SUBTRACT,
            D3D12_BLEND_OP_MIN,
            D3D12_BLEND_OP_MAX
        };
        D3D12_BLEND_OP DX12Backend::nativeBlendOp(BlendOperation op)
        {
            return D3D12_NATIVE_BLEND_OPS[(uint32_t)op];
        }

        D3D12_LOGIC_OP D3D12_NATIVE_LOGIC_OPS[]
        {
            D3D12_LOGIC_OP_CLEAR,
            D3D12_LOGIC_OP_AND,
            D3D12_LOGIC_OP_AND_REVERSE,
            D3D12_LOGIC_OP_COPY,
            D3D12_LOGIC_OP_AND_INVERTED,
            D3D12_LOGIC_OP_NOOP,
            D3D12_LOGIC_OP_XOR,
            D3D12_LOGIC_OP_OR,
            D3D12_LOGIC_OP_NOR,
            D3D12_LOGIC_OP_EQUIV,
            D3D12_LOGIC_OP_INVERT,
            D3D12_LOGIC_OP_AND_REVERSE,
            D3D12_LOGIC_OP_COPY_INVERTED,
            D3D12_LOGIC_OP_OR_INVERTED,
            D3D12_LOGIC_OP_NAND,
            D3D12_LOGIC_OP_SET
        };
        D3D12_LOGIC_OP DX12Backend::nativeLogicOp(LogicOperation op)
        {
            return D3D12_NATIVE_LOGIC_OPS[(uint32_t)op];
        }

        D3D12_FILL_MODE DX12Backend::nativeFillMode(RasterizationState::DrawMode mode)
        {
            switch(mode)
            {
            case RasterizationState::DrawMode::FACE:
                return D3D12_FILL_MODE_SOLID;
            case RasterizationState::DrawMode::EDGE:
            case RasterizationState::DrawMode::VERTEX:
                return D3D12_FILL_MODE_WIREFRAME;
            }
            return D3D12_FILL_MODE_SOLID;
        }

        D3D12_CULL_MODE DX12Backend::nativeCullMode(RasterizationState::CullOptions mode)
        {
            switch(mode)
            {
            case RasterizationState::CullOptions::NONE:
                return D3D12_CULL_MODE_NONE;
                break;
            case RasterizationState::CullOptions::FRONT_FACING:
                return D3D12_CULL_MODE_FRONT;
                break;
            case RasterizationState::CullOptions::BACK_FACING:
                return D3D12_CULL_MODE_BACK;
                break;
            }
            return D3D12_CULL_MODE_NONE;

        }

        D3D12_COMPARISON_FUNC D3D12_NATIVE_COMPARISON_FUNCS[]
        {
            D3D12_COMPARISON_FUNC_NEVER,
            D3D12_COMPARISON_FUNC_LESS,
            D3D12_COMPARISON_FUNC_LESS_EQUAL,
            D3D12_COMPARISON_FUNC_GREATER,
            D3D12_COMPARISON_FUNC_GREATER_EQUAL,
            D3D12_COMPARISON_FUNC_EQUAL,
            D3D12_COMPARISON_FUNC_NOT_EQUAL,
            D3D12_COMPARISON_FUNC_ALWAYS
        };
        D3D12_COMPARISON_FUNC DX12Backend::nativeCompareFunc(ComparisonFunction func)
        {
            return D3D12_NATIVE_COMPARISON_FUNCS[(uint32_t)func];
        }

        D3D12_STENCIL_OP D3D12_NATIVE_STENCIL_OPS[]
        {
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_ZERO,
            D3D12_STENCIL_OP_REPLACE,
            D3D12_STENCIL_OP_INCR_SAT,
            D3D12_STENCIL_OP_DECR_SAT,
            D3D12_STENCIL_OP_INVERT,
            D3D12_STENCIL_OP_INCR,
            D3D12_STENCIL_OP_DECR
        };

        D3D12_DEPTH_STENCILOP_DESC DX12Backend::nativeDepthStencilOpDesc(StencilOpState state)
        {
            D3D12_DEPTH_STENCILOP_DESC desc{};
            desc.StencilFailOp = D3D12_NATIVE_STENCIL_OPS[(uint32_t)state.failOp];
            desc.StencilDepthFailOp = D3D12_NATIVE_STENCIL_OPS[(uint32_t)state.depthFailOp];
            desc.StencilPassOp = D3D12_NATIVE_STENCIL_OPS[(uint32_t)state.passOp];
            desc.StencilFunc = nativeCompareFunc(state.compareOp);
            return desc;
        }

        D3D12_FILTER DX12Backend::nativeFilter(SamplerFilter min, SamplerFilter mag, SamplerFilter mip,
            bool anisotrophyEnabled)
        {
            if(anisotrophyEnabled)
            {
                return D3D12_FILTER_ANISOTROPIC;
            }
            if(min == SamplerFilter::LINEAR)
            {
                if(mag == SamplerFilter::LINEAR)
                {
                    if(mip == SamplerFilter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                    }
                    else if(mip == SamplerFilter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    }
                }
                else if(mag == SamplerFilter::NEAREST)
                {
                    if(mip == SamplerFilter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                    }
                    else if(mip == SamplerFilter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                    }
                }
            }
            else if(min == SamplerFilter::NEAREST)
            {
                if(mag == SamplerFilter::LINEAR)
                {
                    if(mip == SamplerFilter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                    }
                    else if(mip == SamplerFilter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                    }
                }
                else if(mag == SamplerFilter::NEAREST)
                {
                    if(mip == SamplerFilter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                    }
                    else if(mip == SamplerFilter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_MAG_MIP_POINT;
                    }
                }
            }
            return D3D12_FILTER_ANISOTROPIC;
        }

        D3D12_TEXTURE_ADDRESS_MODE DX12Backend::nativeAddressMode(SamplerAddressMode mode)
        {
            switch (mode)
            {
            case SamplerAddressMode::REPEAT:
                return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case SamplerAddressMode::MIRRORED_REPEAT:
                return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case SamplerAddressMode::CLAMP_TO_EDGE:
                return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case SamplerAddressMode::CLAMP_TO_BORDER:
                return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            }
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        }

        D3D12_SRV_DIMENSION DX12Backend::nativeSRVTextureDimension(TextureType type, uint32_t arraySize, SampleCount sampleCount)
        {
            switch (type)
            {
            case TextureType::ONE_DIMENSIONAL:
                return arraySize == 1 ? D3D12_SRV_DIMENSION_TEXTURE1D : D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            case TextureType::TWO_DIMENSIONAL:
                if (sampleCount == SampleCount::ONE)
                {
                    return arraySize == 1 ? D3D12_SRV_DIMENSION_TEXTURE2D : D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                }
                return arraySize == 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            case TextureType::THREE_DIMENSIONAL:
                return D3D12_SRV_DIMENSION_TEXTURE3D;
            case TextureType::CUBE_MAP:
                return arraySize == 6 ? D3D12_SRV_DIMENSION_TEXTURECUBE : D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
            }
            return D3D12_SRV_DIMENSION_UNKNOWN;
        }

        D3D12_UAV_DIMENSION DX12Backend::nativeUAVTextureDimension(TextureType type, uint32_t arraySize,
            SampleCount sampleCount)
        {
            switch (type)
            {
            case TextureType::ONE_DIMENSIONAL:
                return arraySize == 1 ? D3D12_UAV_DIMENSION_TEXTURE1D : D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            case TextureType::TWO_DIMENSIONAL:
                if (sampleCount == SampleCount::ONE)
                {
                    return arraySize == 1 ? D3D12_UAV_DIMENSION_TEXTURE2D : D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                }
                return arraySize == 1 ? D3D12_UAV_DIMENSION_TEXTURE2DMS : D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
            case TextureType::THREE_DIMENSIONAL:
                return D3D12_UAV_DIMENSION_TEXTURE3D;
            }
            return D3D12_UAV_DIMENSION_UNKNOWN;
        }

        SlagInitializationResult DX12Backend::initializeBackend(const InitializationData& initializationData)
        {
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
            UINT createFactoryFlags = 0;
            if(initializationData.debugHandler)
            {
                _debugHandler = initializationData.debugHandler;
                Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface = nullptr;
                auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
                if (result != S_OK)
                {
                    return SlagInitializationResult::INSUFFICIENT_CAPABILITIES;
                }
                Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
                result = debugInterface->QueryInterface(IID_PPV_ARGS(&debugController));
                if (result != S_OK)
                {
                    return SlagInitializationResult::INSUFFICIENT_CAPABILITIES;
                }
                debugController->EnableDebugLayer();
                debugController->SetEnableGPUBasedValidation(true);

                createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
            }

            CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));

            if (!dxgiFactory)
            {
                return SlagInitializationResult::INSUFFICIENT_CAPABILITIES;
            }
            _dxgiFactory = dxgiFactory;

            Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;

            for (UINT i = 0; _dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                // Check to see if the adapter can create a D3D12 device without actually
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                    SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr)))
                {
                    dxgiAdapter1.As(&dxgiAdapter4);
                    Microsoft::WRL::ComPtr<ID3D12Device2> device;
                    D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device));

                    D3D12_FEATURE_DATA_D3D12_OPTIONS12 features{};
                    auto res = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,&features,sizeof(features));
                    if (!features.EnhancedBarriersSupported)
                    {
                        continue;
                    }
                    D3D12_FEATURE_DATA_D3D12_OPTIONS13 features13{};
                    res = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13,&features13,sizeof(features13));
                    if (!features13.UnrestrictedBufferTextureCopyPitchSupported)
                    {
                        continue;
                    }
                    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
                    shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
                    res = device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&shaderModel,sizeof(shaderModel));
                    if (FAILED(res))
                    {
                        continue;
                    }
                    if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
                    {
                        continue;
                    }
                    _graphicsCards.emplace_back(device,dxgiFactory,dxgiAdapter4,initializationData.debugHandler!=nullptr);
                }
            }
            if (_graphicsCards.size() == 0)
            {
                return SlagInitializationResult::NO_GRAPHICS_CARDS;
            }
            return SlagInitializationResult::SUCCESS;
        }

    } // dx12
} // slag
