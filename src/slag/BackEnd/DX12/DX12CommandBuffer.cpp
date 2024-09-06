#include "DX12CommandBuffer.h"
#include "DX12Lib.h"
namespace slag
{
    namespace dx
    {
        DX12CommandBuffer::DX12CommandBuffer(GpuQueue::QueueType commandType)
        {
            _commandType = commandType;
            D3D12_COMMAND_LIST_TYPE comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (_commandType)
            {
                case GpuQueue::Graphics:
                    comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    break;
                case GpuQueue::Transfer:
                    comType = D3D12_COMMAND_LIST_TYPE_COPY;
                    break;
                case GpuQueue::Compute:
                    comType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    break;

            }
            DX12Lib::card()->device()->CreateCommandAllocator(comType, IID_PPV_ARGS(&_pool));
            DX12Lib::card()->device()->CreateCommandList(0,comType,_pool, nullptr, IID_PPV_ARGS(&_buffer));
        }

        DX12CommandBuffer::~DX12CommandBuffer()
        {

            if(_buffer)
            {
                //there's the possibility we started recording commands, but never submitted. Force clear resources just in case
                if(_finished == nullptr)
                {
                    resources::ResourceManager::removeConsumerFromActive(this);
                    freeResourceReferences();
                }
                else
                {
                    _waitUntilFinished();
                }
                _buffer->Release();
                _pool->Release();
                delete _finished;
            }
        }

        DX12CommandBuffer::DX12CommandBuffer(DX12CommandBuffer&& from)
        {
            move(std::move(from));
        }

        DX12CommandBuffer& DX12CommandBuffer::operator=(DX12CommandBuffer&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12CommandBuffer::move(DX12CommandBuffer&& from)
        {
            std::swap(_buffer,from._buffer);
            std::swap(_pool,from._pool);
            std::swap(_finished,from._finished);
        }

        void DX12CommandBuffer::begin()
        {
            _waitUntilFinished();
            resources::ResourceManager::setConsumerAsActive(this);
            _buffer->Reset(_pool, nullptr);
        }

        void DX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        void DX12CommandBuffer::waitUntilFinished()
        {
            _waitUntilFinished();
        }

        bool DX12CommandBuffer::isFinished()
        {
            if(_finished)
            {
                return _finished->value() >=1;
            }
            return true;
        }

        void DX12CommandBuffer::_waitUntilFinished()
        {
            if(_finished)
            {
                _finished->waitForValue(1);
                resources::ResourceManager::removeConsumerFromActive(this);
                freeResourceReferences();
                delete _finished;
                _finished = nullptr;
            }
        }

        GpuQueue::QueueType DX12CommandBuffer::commandType()
        {
            return GpuQueue::Graphics;
        }

        void DX12CommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)
        {
            throw std::runtime_error("not implemented");
        }

        void DX12CommandBuffer::clearColorImage(slag::Texture* texture, slag::ClearColor color, Texture::Layout layout)
        {
            throw std::runtime_error("not implemented");
        }

    } // dx
} // slag