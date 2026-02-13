#include "DX12Backend.h"

namespace slag
{
    namespace dx12
    {
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

        SlagDebugHandlerPtr DX12Backend::getDebugHandler() const
        {
            return _debugHandler;
        }

        SlagInitializationResult DX12Backend::initializeBackend(const InitializationData& initializationData)
        {
            Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
            UINT createFactoryFlags = 0;
            if(initializationData.debugHandler)
            {
                _debugHandler = initializationData.debugHandler;
                Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface = nullptr;
                D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
                Microsoft::WRL::ComPtr<ID3D12Debug1> debugController;
                debugInterface->QueryInterface(IID_PPV_ARGS(&debugController));
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
