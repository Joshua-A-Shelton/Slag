#include "DX12Frame.h"

#include "DX12SwapChain.h"
#include "DX12Texture.h"

namespace slag
{
    namespace dx12
    {
        DX12Frame::DX12Frame(uint32_t frameIndex, ID3D12Resource* texture, uint32_t width, uint32_t height,
            PixelFormat format, DX12SwapChain* parentChain)
        {
            _frameIndex = frameIndex;

            _renderBuffer = new DX12Texture((DX12GraphicsCard*)parentChain->graphicsCard(),texture,format,TextureUsageFlags::COLOR_TARGET,TextureType::TWO_DIMENSIONAL,width,height,1,1,1,SampleCount::ONE);
            _frameBufferView = new DX12FrameBufferView((DX12GraphicsCard*)parentChain->graphicsCard(),_renderBuffer,0,0,1);
            _parentChain = parentChain;
        }

        DX12Frame::~DX12Frame()
        {
            if (_renderBuffer)
            {
                delete _renderBuffer;
                delete _frameBufferView;
            }
        }

        DX12Frame::DX12Frame(DX12Frame&& from) noexcept
        {
            move(from);
        }

        DX12Frame& DX12Frame::operator=(DX12Frame&& from) noexcept
        {
            move(from);
            return *this;
        }

        Texture* DX12Frame::renderBuffer()
        {
            return _renderBuffer;
        }

        FrameBufferView* DX12Frame::defaultView()
        {
            return _frameBufferView;
        }

        void DX12Frame::move(DX12Frame& from)
        {
            _frameIndex = from._frameIndex;
            _parentChain = from._parentChain;
            std::swap(_renderBuffer, from._renderBuffer);
            std::swap(_frameBufferView, from._frameBufferView);
        }
    } // dx12
} // slag