#include "DX12Frame.h"
#include "DX12Lib.h"
#include "DX12Swapchain.h"

namespace slag
{
    namespace dx
    {

        DX12Frame::DX12Frame(ID3D12Resource* texture, Pixels::Format textureFormat, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS textureUsage, DX12Swapchain* from, FrameResources* frameResources): Frame(frameResources)
        {
            _backBuffer = new DX12Texture(texture,false,textureFormat,width,height,1,textureUsage, true);
            _from = from;
        }

        DX12Frame::~DX12Frame()
        {
            delete _backBuffer;
        }

        DX12Frame::DX12Frame(DX12Frame&& from): Frame(nullptr)
        {
            move(std::move(from));
        }

        DX12Frame& DX12Frame::operator=(DX12Frame&& from)
        {
            move(std::move(from));
            return *this;
        }

        Texture* DX12Frame::backBuffer()
        {
            return _backBuffer;
        }

        void DX12Frame::move(DX12Frame&& from)
        {
            Frame::move(from);
            std::swap(_backBuffer,from._backBuffer);
            std::swap(_from, from._from);
        }

        DX12Swapchain* DX12Frame::from()
        {
            return _from;
        }
    } // dx
} // slag