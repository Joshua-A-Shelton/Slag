#include "DX12Swapchain.h"
#include "DX12Lib.h"
#include <comdef.h>

namespace slag
{
    namespace dx
    {
        DX12Swapchain::DX12Swapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, DXGI_FORMAT format)
        {
            _surface = (HWND)platformData.nativeWindowHandle;
            _width = width;
            _height = height;
            _backBuffers = backBuffers;
            _presentMode = mode;
            _format = format;
            rebuild();
        }

        DX12Swapchain::~DX12Swapchain()
        {
        }

        void DX12Swapchain::rebuild()
        {
            if(_swapchain)
            {
                _swapchain = nullptr;
            }

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = _width;
            swapChainDesc.Height = _height;
            swapChainDesc.Format = _format;
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc = { 1, 0 };
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_BACK_BUFFER;
            swapChainDesc.BufferCount = _backBuffers;

            if(_presentMode == PresentMode::Discard)
            {
                swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            }
            else
            {
                swapChainDesc.Scaling = DXGI_SCALING_NONE;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            }
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            // It is recommended to always allow tearing if tearing support is available.
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
            //TODO: I apparently cant use sRGB formats for swapchain images, I think I need to force non sRGB as the image type, and use sRGB format as render target? see bottom of accepted answer on https://gamedev.stackexchange.com/questions/149822/direct3d-12-cant-create-a-swap-chain
            DX12Lib::card()->dxgiFactory()->CreateSwapChainForHwnd(dynamic_cast<DX12Queue*>(DX12Lib::card()->graphicsQueue())->underlyingQueue(),_surface,&swapChainDesc, nullptr, nullptr,&swapChain1);
            //this is the reason we have to wrap the swapchain in the ComPtr, I don't know how to do this without it
            swapChain1.As(&_swapchain);
            //TODO: this may need to change based on the present mode
            //_swapchain->SetMaximumFrameLatency(2);

            _frames.clear();
            for(int i=0; i< _backBuffers; i++)
            {
                ID3D12Resource* backBuffer = nullptr;
                _swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
                //not too sure about D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
                auto frame = DX12Frame(backBuffer,_format,_width,_height,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,this);
                _frames.push_back(std::move(frame));
            }
        }

        Frame* DX12Swapchain::next()
        {
            HANDLE waitOn = _swapchain->GetFrameLatencyWaitableObject();
            WaitForSingleObjectEx(waitOn,1000,true);
            return &_frames[_swapchain->GetCurrentBackBufferIndex()];
        }

        Frame* DX12Swapchain::nextIfReady()
        {
            throw std::runtime_error("not implemented");
            return nullptr;
        }

        Microsoft::WRL::ComPtr<IDXGISwapChain4>& DX12Swapchain::underlyingSwapchain()
        {
            return _swapchain;
        }

        Frame* DX12Swapchain::currentFrame()
        {
            return &_frames[_swapchain->GetCurrentBackBufferIndex()];
        }

        uint8_t DX12Swapchain::currentFrameIndex()
        {
            return _swapchain->GetCurrentBackBufferIndex();
        }

        uint8_t DX12Swapchain::backBuffers()
        {
            return _backBuffers;
        }

        void DX12Swapchain::backBuffers(uint8_t count)
        {
            _backBuffers = count;
            _swapchain->ResizeBuffers(_backBuffers,_width,_height,_format,0);
        }

        uint32_t DX12Swapchain::width()
        {
            return _width;
        }

        uint32_t DX12Swapchain::height()
        {
            return _height;
        }

        void DX12Swapchain::resize(uint32_t width, uint32_t height)
        {
            _width = width;
            _height = height;
            _swapchain->ResizeBuffers(_backBuffers,_width,_height,_format,0);
        }

        Swapchain::PresentMode DX12Swapchain::presentMode()
        {
            return _presentMode;
        }

        void DX12Swapchain::presentMode(Swapchain::PresentMode mode)
        {
            throw std::runtime_error("not implemented");
        }
    } // dx
} // slag