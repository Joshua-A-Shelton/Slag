#include "DX12SwapChain.h"

#include "DX12Backend.h"
#include "DX12SubmissionQueue.h"
#include "slag/exceptions/InvalidSwapChainOperation.h"

namespace slag
{
    namespace dx12
    {
        DX12SwapChain::DX12SwapChain(DX12GraphicsCard* graphicsCard, PlatformData platformData, uint32_t width,
            uint32_t height, const SwapChainParameters& parameters)
        {
            if (platformData.platform != Platform::WIN_32)
            {
                throw std::runtime_error("Attempted to initialize Windows swapchain on non-Windows platform");
            }

            _graphicsCard = graphicsCard;
            _window = platformData.details.win32.hwnd;
            _parameters = parameters;
            _width = width;
            _height = height;
            rebuild();
        }

        DX12SwapChain::~DX12SwapChain()
        {
            if (_swapChain)
            {
                _frames.clear();
            }
        }

        DX12SwapChain::DX12SwapChain(DX12SwapChain&& from) noexcept
        {
            move(from);
        }

        DX12SwapChain& DX12SwapChain::operator=(DX12SwapChain&& from) noexcept
        {
            move(from);
            return *this;
        }

        Frame* DX12SwapChain::next()
        {
            if (_presentRequired)
            {
                throw InvalidSwapChainOperation("Present must be called before calling next again");
            }
            auto wait = _swapChain->GetFrameLatencyWaitableObject();
            DWORD result = WaitForSingleObjectEx(wait,1000,true);
            _presentRequired = true;
            return &_frames[_swapChain->GetCurrentBackBufferIndex()];
        }

        Frame* DX12SwapChain::currentFrame()
        {
            return &_frames[_swapChain->GetCurrentBackBufferIndex()];
        }

        void DX12SwapChain::present()
        {
            if (!_presentRequired)
            {
                throw InvalidSwapChainOperation("Next must be called before present");
            }
            _swapChain->Present(0,0);
            _presentRequired = false;
        }

        const SwapChainParameters& DX12SwapChain::parameters() const
        {
            return _parameters;
        }

        GraphicsCard* DX12SwapChain::graphicsCard()
        {
            return _graphicsCard;
        }

        void DX12SwapChain::move(DX12SwapChain& from)
        {
            std::swap(_window, from._window);
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_swapChain, from._swapChain);
            _parameters = from._parameters;
            _frames.swap(from._frames);
            _width = from._width;
            _height = from._height;
            _presentRequired = from._presentRequired;
        }

        void DX12SwapChain::rebuild()
        {
            if (_swapChain)
            {
                _swapChain = nullptr;
            }
            DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
            swapChainDesc.Width = _width;
            swapChainDesc.Height = _height;
            swapChainDesc.Format = DX12Backend::nativeFormat(_parameters.imageFormat);
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc = { 1, 0 };
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_BACK_BUFFER;
            swapChainDesc.BufferCount = _parameters.imageCount;

            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            //this apparently doesn't have anything to do with double/triple buffering
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

            if (_parameters.presentMode == PresentMode::IMMEDIATE)
            {
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
            }

            swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
            //TODO: I apparently can't use sRGB formats for swapchain images, I think I need to force non sRGB as the image type, and use sRGB format as render target? see bottom of accepted answer on https://gamedev.stackexchange.com/questions/149822/direct3d-12-cant-create-a-swap-chain
            _graphicsCard->dxgiFactory()->CreateSwapChainForHwnd(static_cast<DX12SubmissionQueue*>(_graphicsCard->graphicsQueue())->dx12Handle(),_window,&swapChainDesc, nullptr, nullptr,&swapChain1);
            //this is the reason we have to wrap the swapchain in the ComPtr, I don't know how to do this without it
            swapChain1.As(&_swapChain);

            if (_parameters.presentMode == PresentMode::QUEUE)
            {
                if (_parameters.imageCount > 1)
                {
                    _swapChain->SetMaximumFrameLatency(_parameters.imageCount-1);
                }
                else
                {
                    _swapChain->SetMaximumFrameLatency(1);
                }
            }
            else
            {
                _swapChain->SetMaximumFrameLatency(1);
            }

            _frames.clear();
            for(int i=0; i< _parameters.imageCount; i++)
            {
                ID3D12Resource* backBuffer = nullptr;
                _swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
                DX12Frame frame(i,backBuffer,_width,_height,_parameters.imageFormat,this);
                _frames.push_back(std::move(frame));
            }
        }
    } // dx12
} // slag