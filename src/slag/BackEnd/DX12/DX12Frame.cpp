#include "DX12Frame.h"
#include "DX12Lib.h"
#include "DX12Swapchain.h"

namespace slag
{
    namespace dx
    {

        DX12Frame::DX12Frame(ID3D12Resource* texture, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS textureUsage, DX12Swapchain* from)
        {
            _backBuffer = new DX12Texture(texture,false,textureFormat,width,height,1,textureUsage, true);
            _commandBuffer = new DX12CommandBuffer(GpuQueue::Graphics);
            _from = from;
        }

        DX12Frame::~DX12Frame()
        {
            delete _backBuffer;
            delete _commandBuffer;
        }

        DX12Frame::DX12Frame(DX12Frame&& from)
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

        CommandBuffer* DX12Frame::commandBuffer()
        {
            return _commandBuffer;
        }

        void DX12Frame::move(DX12Frame&& from)
        {
            std::swap(_backBuffer,from._backBuffer);
            std::swap(_commandBuffer,from._commandBuffer);
            std::swap(_from, from._from);
        }

        /*void DX12Frame::begin()
        {
            _commandBuffer->begin();
        }

        void DX12Frame::end()
        {
            _commandBuffer->end();
            DX12Lib::card()->graphicsQueue()->submit(_commandBuffer);
            _commandBuffer->waitUntilFinished();
            //TODO: this may depend on the present mode
            _from->underlyingSwapchain()->Present(0,0);
        }*/
    } // dx
} // slag