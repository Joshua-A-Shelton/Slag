#include "IDX12CommandBuffer.h"

namespace slag
{
    namespace dx
    {
        void IDX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        GpuQueue::QueueType IDX12CommandBuffer::commandType()
        {
            return _commandType;
        }

        void IDX12CommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)
        {
            throw std::runtime_error("not implemented");
        }

        void IDX12CommandBuffer::clearColorImage(slag::Texture* texture, slag::ClearColor color, Texture::Layout layout)
        {
            throw std::runtime_error("not implemented");
        }

        void IDX12CommandBuffer::move(IDX12CommandBuffer& from)
        {
            std::swap(_pool,from._pool);
            std::swap(_buffer,from._buffer);
            _commandType = from._commandType;
        }
    } // dx
} // slag