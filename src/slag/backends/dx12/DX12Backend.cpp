#include "DX12Backend.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#include "core/DX12Buffer.h"
#include "core/DX12CommandBuffer.h"
#include "core/DX12DescriptorPool.h"
#include "core/DX12GraphicsCard.h"
#include "core/DX12Sampler.h"
#include "core/DX12Semaphore.h"
#include "core/DX12ShaderPipeline.h"
#include "core/DX12Texture.h"
#include "slag/core/Pixels.h"

namespace slag
{
    namespace dx12
    {
        std::vector<DXGI_FORMAT> DXGI_FORMATS
        {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits,colorBits,depthBits,stencilBits, aspects) DxName,
            SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        DXGI_FORMAT DX12Backend::dx12Format(Pixels::Format format)
        {
            return(DXGI_FORMATS[static_cast<uint32_t>(format)]);
        }

        D3D12_RESOURCE_DIMENSION DX12Backend::dx12Dimension(Texture::Type type)
        {
            switch (type)
            {
            case Texture::Type::TEXTURE_1D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
            case Texture::Type::TEXTURE_2D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            case Texture::Type::TEXTURE_3D:
                return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            case Texture::Type::TEXTURE_CUBE:
                return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            }
            return D3D12_RESOURCE_DIMENSION_UNKNOWN;
        }

        D3D12_RESOURCE_FLAGS DX12Backend::dx12UsageFlags(Texture::UsageFlags usage)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if ((uint8_t)(usage & Texture::UsageFlags::SAMPLED_IMAGE) == 0)
            {
                //flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
            if((uint8_t)(usage & Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            if((uint8_t)(usage & Texture::UsageFlags::RENDER_TARGET_ATTACHMENT))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if ((uint8_t)(usage & Texture::UsageFlags::STORAGE))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            return flags;
        }

        std::vector<D3D12_BLEND> DX12_BLEND_FACTORS
        {
#define DEFINITION(SlagName, VulkanName, DXName) DXName,
            BLEND_FACTOR_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        D3D12_BLEND DX12Backend::dx12blendFactor(Operations::BlendFactor blendFactor)
        {
            return DX12_BLEND_FACTORS[static_cast<uint8_t>(blendFactor)];
        }

        std::vector<D3D12_BLEND_OP> DX12_BLEND_OPS
        {
#define DEFINITION(SlagName, VulkanName, DXName) DXName,
            BLEND_OP_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        D3D12_BLEND_OP DX12Backend::dx12BlendOp(Operations::BlendOperation blendOperation)
        {
            return DX12_BLEND_OPS[static_cast<uint8_t>(blendOperation)];
        }

        std::vector<D3D12_LOGIC_OP> DX12_LOGIC_OPS
        {
#define DEFINITION(SlagName,VulkanName,DX12Name) DX12Name,
            FRAMEBUFFER_LOGICAL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };

        D3D12_LOGIC_OP DX12Backend::dx12LogicOp(Operations::LogicalOperation operation)
        {
            return DX12_LOGIC_OPS[static_cast<uint8_t>(operation)];
        }

        std::vector<D3D12_FILL_MODE> DX12_FILL_MODES =
        {
            D3D12_FILL_MODE_SOLID,
            D3D12_FILL_MODE_WIREFRAME,//TODO: DX12 has no vertex fill mode, it's pretty pointless anyway, possibly remove it from slag api
            D3D12_FILL_MODE_WIREFRAME
        };

        D3D12_FILL_MODE DX12Backend::dx12FillMode(RasterizationState::DrawMode drawMode)
        {
            return DX12_FILL_MODES[static_cast<uint8_t>(drawMode)];
        }

        std::vector<D3D12_CULL_MODE> DX12_CULL_FLAGS
        {
            D3D12_CULL_MODE_NONE,
            D3D12_CULL_MODE_FRONT,
            D3D12_CULL_MODE_BACK,
        };

        D3D12_CULL_MODE DX12Backend::dx12CullMode(RasterizationState::CullOptions cullOptions)
        {
            return DX12_CULL_FLAGS[static_cast<uint8_t>(cullOptions)];
        }

        std::vector<D3D12_COMPARISON_FUNC> DX12_COMPARE_OPS
        {
#define DEFINITION(slagName, vulkanName, dx12Name) dx12Name,
            COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
        };

        D3D12_COMPARISON_FUNC DX12Backend::dx12ComparisonFunction(Operations::ComparisonFunction comparisonFunction)
        {
            return DX12_COMPARE_OPS[static_cast<uint8_t>(comparisonFunction)];
        }

        D3D12_DEPTH_STENCILOP_DESC DX12Backend::dx12StencilOpDesc(StencilOpState state)
        {
            D3D12_DEPTH_STENCILOP_DESC desc{};
            desc.StencilFailOp = dx12StencilOp(state.failOp);
            desc.StencilDepthFailOp = dx12StencilOp(state.depthFailOp);
            desc.StencilPassOp = dx12StencilOp(state.passOp);
            desc.StencilFunc = dx12ComparisonFunction(state.compareOp);
            return desc;
        }

        std::vector<D3D12_STENCIL_OP> DX12_STENCIL_OPS
        {
#define DEFINITION(SlagName,VulkanName, DX12Name) DX12Name,
            STENCIL_OP_DEFINITIONS(DEFINITION)
#undef DEFINITION
        };

        D3D12_STENCIL_OP DX12Backend::dx12StencilOp(Operations::StencilOperation operation)
        {
            return DX12_STENCIL_OPS[static_cast<uint8_t>(operation)];
        }

        std::vector<DXGI_FORMAT> DX12Backend::dx12GraphicsType(GraphicsType type)
        {
            std::vector<DXGI_FORMAT> formats;
            switch(type)
            {
                case GraphicsType::UNKNOWN:
                    formats.push_back(DXGI_FORMAT_UNKNOWN);
                    break;
                case GraphicsType::BOOLEAN:
                    formats.push_back(DXGI_FORMAT_R8_UINT);
                    break;
                case GraphicsType::INTEGER:
                    formats.push_back(DXGI_FORMAT_R32_SINT);
                    break;
                case GraphicsType::UNSIGNED_INTEGER:
                    formats.push_back(DXGI_FORMAT_R32_UINT);
                    break;
                case GraphicsType::FLOAT:
                    formats.push_back(DXGI_FORMAT_R32_FLOAT);
                    break;
                case GraphicsType::DOUBLE:
                    formats.push_back(DXGI_FORMAT_R32G32_UINT);//notice we have to pass in as two components, each with 32 bits
                    break;
                case GraphicsType::VECTOR2:
                    formats.push_back(DXGI_FORMAT_R32G32_FLOAT);
                    break;
                case GraphicsType::VECTOR3:
                    formats.push_back(DXGI_FORMAT_R32G32B32_FLOAT);
                    break;
                case GraphicsType::VECTOR4:
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_FLOAT);
                    break;
                case GraphicsType::BOOLEAN_VECTOR2:
                    formats.push_back(DXGI_FORMAT_R8G8_UINT);
                    break;
                case GraphicsType::BOOLEAN_VECTOR3:
                    formats.push_back(DXGI_FORMAT_R8_UINT);
                    formats.push_back(DXGI_FORMAT_R8_UINT);
                    formats.push_back(DXGI_FORMAT_R8_UINT);
                    break;
                case GraphicsType::BOOLEAN_VECTOR4:
                    formats.push_back(DXGI_FORMAT_R8G8B8A8_UINT);
                    break;
                case GraphicsType::INTEGER_VECTOR2:
                    formats.push_back(DXGI_FORMAT_R32G32_SINT);
                    break;
                case GraphicsType::INTEGER_VECTOR3:
                    formats.push_back(DXGI_FORMAT_R32G32B32_SINT);
                    break;
                case GraphicsType::INTEGER_VECTOR4:
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_SINT);
                    break;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR2:
                    formats.push_back(DXGI_FORMAT_R32G32_UINT);
                    break;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR3:
                    formats.push_back(DXGI_FORMAT_R32G32B32_UINT);
                    break;
                case GraphicsType::UNSIGNED_INTEGER_VECTOR4:
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_UINT);
                    break;
                case GraphicsType::DOUBLE_VECTOR2:
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_UINT);//notice we have to pass in as two components, each with 32 bits for each component
                    break;
                case GraphicsType::DOUBLE_VECTOR3:
                    formats.push_back(DXGI_FORMAT_R32G32_UINT);//notice we have to pass in as two components, each with 32 bits for each component
                    formats.push_back(DXGI_FORMAT_R32G32_UINT);
                    formats.push_back(DXGI_FORMAT_R32G32_UINT);
                    break;
                case GraphicsType::DOUBLE_VECTOR4:
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_UINT);//notice we have to pass in as two components, each with 32 bits for each component
                    formats.push_back(DXGI_FORMAT_R32G32B32A32_UINT);
                    break;
            }
            return formats;
        }

        uint32_t DX12Backend::dx12FormatSize(DXGI_FORMAT format)
        {
            switch (format)
            {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits, colorBits, depthBits, stencilBits, aspectFlags) case DxName: return totalBits/8;
                SLAG_TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return 0;
        }

        //TODO: this whole function is... a best guess, there's a lot of difference between how DX handles this and Vulkan handles it
        D3D12_FILTER DX12Backend::dx12Filter(Sampler::Filter minFilter, Sampler::Filter magFilter,Sampler::Filter mipMapFilter, bool ansitrophyEnabled)
        {
            if(ansitrophyEnabled)
            {
                return D3D12_FILTER_ANISOTROPIC;
            }
            if(minFilter == Sampler::Filter::LINEAR)
            {
                if(magFilter == Sampler::Filter::LINEAR)
                {
                    if(mipMapFilter == Sampler::Filter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                    }
                    else if(mipMapFilter == Sampler::Filter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    }
                }
                else if(magFilter == Sampler::Filter::NEAREST)
                {
                    if(mipMapFilter == Sampler::Filter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
                    }
                    else if(mipMapFilter == Sampler::Filter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
                    }
                }
            }
            else if(minFilter == Sampler::Filter::NEAREST)
            {
                if(magFilter == Sampler::Filter::LINEAR)
                {
                    if(mipMapFilter == Sampler::Filter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
                    }
                    else if(mipMapFilter == Sampler::Filter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
                    }
                }
                else if(magFilter == Sampler::Filter::NEAREST)
                {
                    if(mipMapFilter == Sampler::Filter::LINEAR)
                    {
                        return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
                    }
                    else if(mipMapFilter == Sampler::Filter::NEAREST)
                    {
                        return D3D12_FILTER_MIN_MAG_MIP_POINT;
                    }
                }
            }
            return D3D12_FILTER_ANISOTROPIC;
        }

        D3D12_TEXTURE_ADDRESS_MODE DX12Backend::dx12AddressMode(Sampler::AddressMode mode)
        {
            switch(mode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::AddressMode::slagName: return dx12Name;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        }



        DX12Backend::DX12Backend(SlagInitInfo initInfo)
        {
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
            UINT createFactoryFlags = 0;
            if(initInfo.slagDebugHandler)
            {
                _dx12DebugHandler = initInfo.slagDebugHandler;
                Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface = nullptr;
                D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
                Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
                debugInterface->QueryInterface(IID_PPV_ARGS(&debugController));
                debugController->EnableDebugLayer();
                debugController->SetEnableGPUBasedValidation(true);

                createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
            }

            CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));


            if(!dxgiFactory)
            {
                _valid = false;
            }
            else
            {
                _dxgiFactory = dxgiFactory;
                _valid = true;
            }


        }

        DX12Backend::~DX12Backend()
        {
        }

        void DX12Backend::postGraphicsCardChosenSetup()
        {
        }

        void DX12Backend::preGraphicsCardDestroyCleanup()
        {
        }

        bool DX12Backend::valid()
        {
            return _valid;
        }

        std::vector<std::unique_ptr<GraphicsCard>> DX12Backend::getGraphicsCards()
        {
            std::vector<std::unique_ptr<GraphicsCard>> cards;



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
                    auto card = std::make_unique<DX12GraphicsCard>(dxgiAdapter4,_dxgiFactory);
                    if (card->isValidGraphicsCard())
                    {
                        cards.push_back(std::move(card));
                    }
                }
            }

            return cards;
        }

        GraphicsBackend DX12Backend::backendAPI()
        {
            return GraphicsBackend::DX12_GRAPHICS_BACKEND;
        }

        CommandBuffer* DX12Backend::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
        {
            return new DX12CommandBuffer(acceptsCommands);
        }

        CommandBuffer* DX12Backend::newSubCommandBuffer(CommandBuffer* parentBuffer)
        {
            throw std::runtime_error("DX12Backend::newSubCommandBuffer() not implemented");
        }

        Semaphore* DX12Backend::newSemaphore(uint64_t initialValue)
        {
            return new DX12Semaphore(initialValue);
        }

        void DX12Backend::waitFor(SemaphoreValue* values, size_t count)
        {
            DX12Semaphore::waitFor(values, count);
        }

        Texture* DX12Backend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers,Texture::SampleCount sampleCount)
        {
            return new DX12Texture(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount);
        }

        Texture* DX12Backend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers,Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings,uint32_t mappingCount)
        {
            return new DX12Texture(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount,texelData,texelDataLength,mappings,mappingCount);
        }

        Buffer* DX12Backend::newBuffer(size_t size, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            return new DX12Buffer(size,accessibility,usage);
        }

        Buffer* DX12Backend::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)
        {
            return new DX12Buffer(data,dataSize,accessibility,usage);
        }

        BufferView* DX12Backend::newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)
        {
            throw std::runtime_error("DX12Backend::newBufferView() not implemented");
        }

        SwapChain* DX12Backend::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height,SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format,SwapChain::AlphaCompositing compositing,FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void(* swapchainRebuiltFunction)(SwapChain* swapChain))
        {
            throw std::runtime_error("DX12Backend::newSwapChain() not implemented");
        }

        Sampler* DX12Backend::newSampler(SamplerParameters parameters)
        {
            return new DX12Sampler(parameters);
        }

        std::vector<ShaderCode::CodeLanguage> DX12Backend::acceptedLanuages()
        {
            std::vector<ShaderCode::CodeLanguage> result{ShaderCode::CodeLanguage::DXIL};
            return result;
        }

        ShaderPipeline* DX12Backend::newShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData)
        {
            return new DX12ShaderPipeline(shaders, shaderCount, properties, vertexDescription, framebufferDescription,rename,renameData);
        }

        ShaderPipeline* DX12Backend::newShaderPipeline(const ShaderCode& computeShader,std::string(*rename)(const std::string&,uint32_t,Descriptor::Type,Descriptor::Dimension,uint32_t, uint32_t,void*), void* renameData)
        {
            throw std::runtime_error("DX12Backend::newShaderPipeline() not implemented");
        }

        DescriptorPool* DX12Backend::newDescriptorPool()
        {
            return new DX12DescriptorPool(1000000);
        }

        DescriptorPool* DX12Backend::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            uint64_t poolSize = 0;

            poolSize += pageInfo.samplers;
            poolSize += pageInfo. sampledTextures;
            poolSize += pageInfo. combinedSamplerTextures;
            poolSize += pageInfo. storageTextures;
            poolSize += pageInfo. uniformTexelBuffers;
            poolSize += pageInfo. storageTexelBuffers;
            poolSize += pageInfo. uniformBuffers;
            poolSize += pageInfo. storageBuffers;
            poolSize += pageInfo. inputAttachments;
            poolSize += pageInfo. accelerationStructures;
            poolSize += pageInfo. descriptorBundles;
            return new DX12DescriptorPool(poolSize);
        }

        void DX12Backend::setDescriptorBundleSampler(DescriptorBundle& descriptor, DescriptorIndex* index,
            uint32_t arrayElement, Sampler* sampler)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleSampler() not implemented");
        }

        void DX12Backend::setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, DescriptorIndex* index,
            uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleSampledTexture() not implemented");
        }

        void DX12Backend::setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, DescriptorIndex* index,
            uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageTexture() not implemented");
        }

        void DX12Backend::setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, BufferView* bufferView)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleUniformTexelBuffer() not implemented");
        }


        void DX12Backend::setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, BufferView* bufferView)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageTexelBuffer() not implemented");
        }

        void DX12Backend::setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, DescriptorIndex* index,
                                                           uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleUniformBuffer() not implemented");
        }

        void DX12Backend::setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, DescriptorIndex* index,
                                                           uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageBuffer() not implemented");
        }

        PixelFormatProperties DX12Backend::pixelFormatProperties(Pixels::Format format)
        {
            PixelFormatProperties properties{};
            if (format == Pixels::Format::UNDEFINED)
            {
                properties.tiling = PixelFormatProperties::Tiling::UNSUPPORTED;
                return properties;
            }

            auto dxformat = DX12Backend::dx12Format(format);
            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport{dxformat};
            DX12GraphicsCard::selected()->device()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&formatSupport,sizeof(formatSupport));
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_NONE )
            {
                return properties;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.validUsageFlags |= Texture::UsageFlags::SAMPLED_IMAGE;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET)
            {
                properties.validUsageFlags |= Texture::UsageFlags::RENDER_TARGET_ATTACHMENT;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL)
            {
                properties.validUsageFlags |= Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW)
            {
                properties.validUsageFlags |= Texture::UsageFlags::STORAGE;
            }

            //TODO: not sure if this is right or not.... but there's not great documentation on which allow linear or not
            //https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/checking-hardware-feature-support
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.linearFilteringCapable = true;
            }
            else
            {
                properties.linearFilteringCapable = false;
            }

            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
            {
                properties.blitSource = true;
            }
            if (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET)
            {
                properties.blitDestination = true;
            }

            //TODO: not sure about this... I think it's right, but could easily be wrong
            if (formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_TILED)
            {
                properties.tiling = PixelFormatProperties::Tiling::OPTIMIZED;
            }
            else
            {
                properties.tiling = PixelFormatProperties::Tiling::LINEAR;
            }

            return properties;
        }


    } // dx12
} // slag
