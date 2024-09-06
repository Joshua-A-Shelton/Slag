#ifndef SLAG_IDX12COMMANDBUFFER_H
#define SLAG_IDX12COMMANDBUFFER_H

#include <d3d12.h>
#include "../../CommandBuffer.h"

namespace slag
{
    namespace dx
    {

        class IDX12CommandBuffer: public CommandBuffer
        {
        public:
            virtual ~IDX12CommandBuffer()=default;
            GpuQueue::QueueType commandType()override;
            void end()override;

            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)override;
        protected:
            void move(IDX12CommandBuffer& from);
            GpuQueue::QueueType _commandType = GpuQueue::Graphics;
            ID3D12GraphicsCommandList* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
        };

    } // dx
} // slag

#endif //SLAG_IDX12COMMANDBUFFER_H
