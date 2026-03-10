#include "DX12Backend.h"

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
            if((uint8_t)(usage & TextureUsageFlags::DEPTH_STENCIL_TARGET))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            }
            if((uint8_t)(usage & TextureUsageFlags::COLOR_TARGET))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            }
            if ((uint8_t)(usage & TextureUsageFlags::UNORDERED_ACCESS))
            {
                flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            return flags;
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
