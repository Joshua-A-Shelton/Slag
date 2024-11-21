#include "DX12Lib.h"
#include "DX12Semaphore.h"
#include "DX12CommandBuffer.h"
#include "DX12Queue.h"
#include "DX12Texture.h"
#include "DX12Swapchain.h"
#include "DX12Buffer.h"
#include "DX12Sampler.h"
#include "DX12Shader.h"
#include "DX12DescriptorGroup.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>


namespace slag
{
    namespace dx
    {

        DX12Lib* DX12Lib::initialize(const SlagInitDetails& details)
        {
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
            UINT createFactoryFlags = 0;
#ifndef NDEBUG
            Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface = nullptr;
            D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
            Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
            debugInterface->QueryInterface(IID_PPV_ARGS(&debugController));
            debugController->EnableDebugLayer();
            debugController->SetEnableGPUBasedValidation(true);

            createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

#endif
            CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));

            Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
            Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;

            bool useWarp = false;
            if (useWarp)
            {
                dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1));
                dxgiAdapter1.As(&dxgiAdapter4);
            }
            else
            {
                SIZE_T maxDedicatedVideoMemory = 0;
                for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
                {
                    DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                    dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                    // Check to see if the adapter can create a D3D12 device without actually
                    // creating it. The adapter with the largest dedicated video memory
                    // is favored.
                    if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                        SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),D3D_FEATURE_LEVEL_12_2, __uuidof(ID3D12Device), nullptr)) &&dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory )
                    {
                        maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                        dxgiAdapter1.As(&dxgiAdapter4);
                    }
                }
            }
            if(!dxgiAdapter4)
            {
                return nullptr;
            }
            mapFlags();
            auto card = new DX12GraphicsCard(dxgiAdapter4,dxgiFactory,details);
            return new DX12Lib(card);
        }

        void DX12Lib::cleanup(lib::BackEndLib* library)
        {
            delete library;
#ifndef NDEBUG
            Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
            if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
            {
                dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            }
#endif

        }

        DXGI_FORMAT DX12Lib::format(Pixels::Format pixelFormat)
        {
            switch(pixelFormat)
            {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits) case Pixels::SlagName: return DxName;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return DXGI_FORMAT_UNKNOWN;
        }

        D3D12_RESOURCE_DIMENSION DX12Lib::dimension(Texture::Type type)
        {
            switch (type)
            {
                case Texture::Type::TEXTURE_1D:
                    return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
                case Texture::Type::TEXTURE_2D:
                    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                case Texture::Type::TEXTURE_3D:
                    return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
                case Texture::Type::CUBE_MAP:
                    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            }
            return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        }

        D3D12_BARRIER_LAYOUT DX12Lib::barrierLayout(Texture::Layout texLayout)
        {
            switch(texLayout)
            {
#define DEFINITION(slagName, vulkanName, directXName, directXResourceName) case Texture::Layout::slagName: return directXName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return  D3D12_BARRIER_LAYOUT_UNDEFINED;
        }

        D3D12_RESOURCE_STATES DX12Lib::stateLayout(Texture::Layout texLayout)
        {
            switch(texLayout)
            {
#define DEFINITION(slagName, vulkanName, directXName, directXResourceName) case Texture::Layout::slagName: return directXResourceName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return  D3D12_RESOURCE_STATE_COMMON;
        }

        D3D12_COMPARISON_FUNC DX12Lib::comparisonFunction(Operations::ComparisonFunction compFunction)
        {
            switch (compFunction)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Operations::slagName:return dx12Name;
                COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
            }
            return D3D12_COMPARISON_FUNC_NONE;
        }

        uint32_t DX12Lib::formatSize(DXGI_FORMAT format)
        {
            switch (format)
            {
#define DEFINITION(SlagName, DxName, VulkanName, VkImageAspectFlags, VkComponentSwizzle_r, VkComponentSwizzle_g, VkComponentSwizzle_b, VkComponentSwizzle_a, totalBits) case DxName: return totalBits/8;
                TEXTURE_FORMAT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return 0;
        }

        D3D12_DESCRIPTOR_RANGE_TYPE DX12Lib::rangeType(Descriptor::DescriptorType type)
        {
            switch (type)
            {
                case Descriptor::SAMPLER:
                case Descriptor::SAMPLER_AND_TEXTURE:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                case Descriptor::STORAGE_TEXTURE:
                case Descriptor::STORAGE_TEXEL_BUFFER:
                case Descriptor::STORAGE_BUFFER:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                case Descriptor::INPUT_ATTACHMENT:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                default:
                    return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

            }
        }

        D3D12_TEXTURE_ADDRESS_MODE DX12Lib::addressMode(Sampler::AddressMode mode)
        {
            switch(mode)
            {
#define DEFINITION(slagName, vulkanName, dx12Name) case Sampler::slagName: return dx12Name;
                SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        }

        //TODO: this whole function is... a best guess, there's a lot of difference between how DX handles this and Vulkan handles it
        D3D12_FILTER DX12Lib::filter(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, bool ansitrophyEnabled)
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

        DX12Lib* DX12Lib::get()
        {
            return dynamic_cast<DX12Lib*>(lib::BackEndLib::get());
        }

        DX12GraphicsCard* DX12Lib::card()
        {
            return get()->_graphicsCard;
        }

        DX12Lib::DX12Lib(DX12GraphicsCard* card)
        {
            _graphicsCard = card;
        }

        DX12Lib::~DX12Lib()
        {
            delete _graphicsCard;
        }

        void DX12Lib::mapFlags()
        {

#define DEFINITION(slagName, vulkanName, directXName) TextureUsageFlags::set##slagName##Value(static_cast<int>(directXName));
            TEXTURE_USAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) BarrierAccessFlags::set##slagName##Value(static_cast<int>(directXName));
            MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) PipelineStageFlags::set##slagName##Value(static_cast<int>(directXName));
            MEMORY_PIPELINE_STAGE_DEFINITIONS(DEFINITION)
#undef DEFINITION

#define DEFINITION(slagName, vulkanName, directXName) ShaderStageFlags::set##slagName##Value(static_cast<int>(directXName));
            SHADER_STAGE_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        }

        BackEnd DX12Lib::identifier()
        {
            return DirectX12;
        }

        GraphicsCard* DX12Lib::graphicsCard()
        {
            return _graphicsCard;
        }

        void DX12Lib::waitFor(SemaphoreValue* values, size_t count)
        {
            DX12Semaphore::waitFor(values,count);
        }

        DX12GraphicsCard* DX12Lib::dx12GraphicsCard()
        {
            return _graphicsCard;
        }

        Texture* DX12Lib::newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels,
                                      TextureUsage usage, Texture::Layout initializedLayout)
        {
            return new DX12Texture(texelDataArray,texelDataCount,dataSize,dataFormat,type,width,height,mipLevels,std::bit_cast<D3D12_RESOURCE_FLAGS>(usage),initializedLayout,false);
        }

        Texture* DX12Lib::newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage)
        {
            return new DX12Texture(dataFormat,type,width,height,mipLevels,layers,sampleCount,std::bit_cast<D3D12_RESOURCE_FLAGS>(usage),false);
        }

        CommandBuffer* DX12Lib::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
        {
            return new DX12CommandBuffer(acceptsCommands);
        }

        Buffer* DX12Lib::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ;
            }
            return new DX12Buffer(data,dataSize,accessibility,states, false);
        }

        Buffer* DX12Lib::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
            if(usage & Buffer::Usage::VERTEX_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ;
            }
            if(usage & Buffer::Usage::INDEX_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            if(usage & Buffer::Usage::STORAGE_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if(usage & Buffer::Usage::INDIRECT_BUFFER)
            {
                states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ;
            }
            return new DX12Buffer(bufferSize,accessibility,states, false);
        }

        Semaphore* DX12Lib::newSemaphore(uint64_t startingValue)
        {
            return new DX12Semaphore(startingValue);
        }

        Swapchain* DX12Lib::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)
        {
            return new DX12Swapchain(platformData,width,height,backBuffers,mode, format(imageFormat));
        }

        Sampler* DX12Lib::newSampler(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, Sampler::AddressMode u, Sampler::AddressMode v, Sampler::AddressMode w,
                                     float mipLODBias, bool enableAnisotrophy, uint8_t maxAnisotrophy, Operations::ComparisonFunction comparisonFunction, Color borderColor, float minLOD, float maxLOD)
        {
            return new DX12Sampler(minFilter,magFilter,mipMapFilter,u,v,w,mipLODBias,enableAnisotrophy,maxAnisotrophy,comparisonFunction,borderColor,minLOD,maxLOD,false);
        }

        DescriptorGroup* DX12Lib::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            return new DX12DescriptorGroup(descriptors, descriptorCount);
        }

        Shader* DX12Lib::newShader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)
        {
            throw std::runtime_error("DX12Lib::newShader not implemented");
        }

    } // dx
} // slag