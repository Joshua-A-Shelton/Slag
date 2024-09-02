#include "DX12Lib.h"
#include "DX12Semaphore.h"
#include "DX12CommandBuffer.h"
#include "DX12Queue.h"
#include <wrl.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>

namespace slag
{
    namespace dx
    {

        DX12Lib* DX12Lib::initialize()
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
            D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
            debugInterface->EnableDebugLayer();

            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
            UINT createFactoryFlags = 0;
#ifndef NDEBUG
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

            auto card = new DX12GraphicsCard(dxgiAdapter4);
            return new DX12Lib(card);
        }

        void DX12Lib::cleanup(lib::BackEndLib* library)
        {
            delete library;
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

        Texture* DX12Lib::newTexture(void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout, bool generateMips)
        {
            throw std::runtime_error("implement");
            return nullptr;
        }

        Texture* DX12Lib::newTexture(CommandBuffer* onBuffer, void* texelData, size_t dataSize, Pixels::Format dataFormat, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, Texture::Usage usage, Texture::Layout initializedLayout, bool generateMips)
        {
            throw std::runtime_error("implement");
            return nullptr;
        }

        CommandBuffer* DX12Lib::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
        {
            D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (acceptsCommands)
            {
                case GpuQueue::Graphics:
                    type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    break;
                case GpuQueue::Transfer:
                    type = D3D12_COMMAND_LIST_TYPE_COPY;
                    break;
                case GpuQueue::Compute:
                    type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    break;

            }
            return new DX12CommandBuffer(type);
        }

        Semaphore* DX12Lib::newSemaphore(uint64_t startingValue)
        {
            return new DX12Semaphore(startingValue);
        }

        Swapchain* DX12Lib::newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)
        {
            throw new std::runtime_error("not implemented");
            return nullptr;
        }

    } // dx
} // slag