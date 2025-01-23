#include "DX12Swapchain.h"
#include "DX12Lib.h"
#include <comdef.h>

namespace slag
{
    namespace dx
    {
        DX12Swapchain::DX12Swapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format format,FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain)):Swapchain(createResourceFunction)
        {
            if(platformData.platform!= PlatformData::Platform::WIN_32)
            {
                throw std::runtime_error("Cannot initialize DX12 Swapchain on any backend except Windows");
            }
            _surface = (HWND)platformData.data.win32.hwnd;
            _width = width;
            _height = height;
            _backBuffers = backBuffers;
            _presentMode = mode;
            _format = format;
            rebuild();
        }

        DX12Swapchain::~DX12Swapchain()
        {
            for(int i=0; i< _frames.size(); i++)
            {
                if(_frames[i].resources)
                {
                    _frames[i].resources->waitForResourcesToFinish();
                }
            }
            DX12Semaphore semaphore(0,true);
            static_cast<DX12Queue*>(DX12Lib::card()->graphicsQueue())->signal(&semaphore,1);
            semaphore.waitForValue(1);
            _swapchain->Release();
        }

        DX12Swapchain::DX12Swapchain(DX12Swapchain&& from): Swapchain(nullptr)
        {
            move(std::move(from));
        }

        DX12Swapchain& DX12Swapchain::operator=(DX12Swapchain&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12Swapchain::move(DX12Swapchain&& from)
        {
            Swapchain::move(from);
            std::swap(_surface,from._surface);
            std::swap(_swapchain,from._swapchain);
            _width=from._width;
            _height=from._height;
            _backBuffers=from._backBuffers;
            _presentMode = from._presentMode;
            _format = from._format;

            _frames.swap(from._frames);
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
            swapChainDesc.Format = DX12Lib::format(_format);
            swapChainDesc.Stereo = false;
            swapChainDesc.SampleDesc = { 1, 0 };
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_BACK_BUFFER;
            swapChainDesc.BufferCount = _backBuffers;

            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            //this apparently doesn't have anything to do with double/triple buffering
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
            //TODO: I should allow/ disallow tearing if vsync is enabled
            swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

            Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain1;
            //TODO: I apparently cant use sRGB formats for swapchain images, I think I need to force non sRGB as the image type, and use sRGB format as render target? see bottom of accepted answer on https://gamedev.stackexchange.com/questions/149822/direct3d-12-cant-create-a-swap-chain
            DX12Lib::card()->dxgiFactory()->CreateSwapChainForHwnd(static_cast<DX12Queue*>(DX12Lib::card()->graphicsQueue())->underlyingQueue(),_surface,&swapChainDesc, nullptr, nullptr,&swapChain1);
            //this is the reason we have to wrap the swapchain in the ComPtr, I don't know how to do this without it
            swapChain1.As(&_swapchain);

            _swapchain->SetMaximumFrameLatency(_backBuffers-1);

            _frames.clear();
            for(int i=0; i< _backBuffers; i++)
            {
                ID3D12Resource* backBuffer = nullptr;
                FrameResources* fr = nullptr;
                if(createResources)
                {
                    fr = createResources(i,this);
                }
                _swapchain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
                //not too sure about D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE
                auto frame = DX12Frame(backBuffer,_format,_width,_height,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE,this,fr);
                _frames.push_back(std::move(frame));
            }
        }

        Frame* DX12Swapchain::next()
        {
            auto frame = &_frames[_swapchain->GetCurrentBackBufferIndex()];
            frame->resources->waitForResourcesToFinish();
            return frame;
        }

        Frame* DX12Swapchain::nextIfReady()
        {
            auto frame = &_frames[_swapchain->GetCurrentBackBufferIndex()];
            if(frame->resources)
            {
                if(frame->resources->isFinished())
                {
                    return frame;
                }
                return nullptr;
            }
            return frame;
        }

        IDXGISwapChain4* DX12Swapchain::underlyingSwapchain()
        {
            return _swapchain.Get();
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
            _swapchain->ResizeBuffers(_backBuffers,_width,_height,DX12Lib::format(_format),0);
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
            _swapchain->ResizeBuffers(_backBuffers,_width,_height,DX12Lib::format(_format),0);
            _swapchain->SetMaximumFrameLatency(_backBuffers-1);
        }

        Swapchain::PresentMode DX12Swapchain::presentMode()
        {
            return _presentMode;
        }

        void DX12Swapchain::presentMode(Swapchain::PresentMode mode)
        {
            _presentMode = mode;
        }

    } // dx
} // slag