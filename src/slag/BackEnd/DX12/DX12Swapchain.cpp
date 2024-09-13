#include "DX12Swapchain.h"
#include "DX12Lib.h"

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
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = _backBuffers;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            if(_presentMode == PresentMode::Discard)
            {
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            }
            else
            {
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            }
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            // It is recommended to always allow tearing if tearing support is available.
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT | DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFSDesc = {};
            swapChainFSDesc.Windowed = true;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
            auto result = DX12Lib::card()->dxgiFactory()->CreateSwapChainForHwnd(dynamic_cast<DX12Queue*>(DX12Lib::card()->graphicsQueue())->underlyingQueue(),_surface,&swapChainDesc, &swapChainFSDesc, nullptr,&swapChain1);
            //this is the reason we have to wrap the swapchain in the ComPtr, I don't know how to do this without it
            swapChain1.As(&_swapchain);
            //TODO: this may need to change based on the present mode
            //_swapchain->SetMaximumFrameLatency(2);
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