#include "DX12Lib.h"
#include "DX12Semaphore.h"
#include "DX12CommandBuffer.h"
#include "DX12Queue.h"
#include "DX12Texture.h"
#include "DX12Swapchain.h"
#include "DX12Buffer.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

namespace slag
{
    namespace dx
    {

        DX12Lib* DX12Lib::initialize()
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
                        SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory )
                    {
                        maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                        dxgiAdapter1.As(&dxgiAdapter4);
                    }
                }
            }

            auto card = new DX12GraphicsCard(dxgiAdapter4,dxgiFactory);
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

        D3D12_BARRIER_LAYOUT DX12Lib::layout(Texture::Layout texLayout)
        {
            switch(texLayout)
            {
#define DEFINITION(slagName, vulkanName, directXName) case Texture::Layout::slagName: return directXName;
                TEXTURE_LAYOUT_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            }
            return  D3D12_BARRIER_LAYOUT_UNDEFINED;
        }

        D3D12_RESOURCE_FLAGS DX12Lib::usage(Texture::Usage texUsage)
        {
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
            if(!(texUsage & Texture::SAMPLED_IMAGE))
            {
                flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
            }
            if(texUsage & Texture::STORAGE)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            if(texUsage & Texture::RENDER_TARGET_ATTACHMENT)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if(texUsage & Texture::DEPTH_STENCIL_ATTACHMENT)
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            return flags;
        }

        D3D12_BARRIER_ACCESS DX12Lib::access(BarrierAccess barrierAccess)
        {
            D3D12_BARRIER_ACCESS flags = static_cast<D3D12_BARRIER_ACCESS>(0);
#define DEFINITION(slagName, slagValue, vulkanName, directXName)  if (barrierAccess & slagName){ flags |= directXName;}
            MEMORY_BARRIER_ACCESS_DEFINTITIONS(DEFINITION)
#undef DEFINITION
            return flags;
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

        Texture* DX12Lib::newTexture(void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage texUsage, Texture::Layout initializedLayout, bool generateMips)
        {
            return new DX12Texture(texelData,dataSize, format(dataFormat), format(textureFormat),width, height, mipLevels,usage(texUsage), layout(initializedLayout),generateMips,false);
        }

        Texture* DX12Lib::newTexture(CommandBuffer* onBuffer, void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage texUsage, Texture::Layout initializedLayout, bool generateMips)
        {
            return new DX12Texture(dynamic_cast<DX12CommandBuffer*>(onBuffer),texelData,dataSize, format(dataFormat), format(textureFormat),width, height, mipLevels,usage(texUsage), layout(initializedLayout),generateMips,false);
        }

        CommandBuffer* DX12Lib::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
        {
            return new DX12CommandBuffer(acceptsCommands);
        }

        Buffer* DX12Lib::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
            if(usage & Buffer::Usage::VertexBuffer)
            {
                states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ;
            }
            if(usage & Buffer::Usage::IndexBuffer)
            {
                states |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            if(usage & Buffer::Usage::Storage)
            {
                states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if(usage & Buffer::Usage::Indirect)
            {
                states |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT ;
            }
            return new DX12Buffer(data,dataSize,accessibility,states, false);
        }

        Buffer* DX12Lib::newBuffer(size_t bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)
        {
            D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;
            if(usage & Buffer::Usage::VertexBuffer)
            {
                states |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ;
            }
            if(usage & Buffer::Usage::IndexBuffer)
            {
                states |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
            }
            if(usage & Buffer::Usage::Storage)
            {
                states |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            }
            if(usage & Buffer::Usage::Indirect)
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

    } // dx
} // slag