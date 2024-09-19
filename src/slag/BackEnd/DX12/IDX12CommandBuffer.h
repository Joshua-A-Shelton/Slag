#ifndef SLAG_IDX12COMMANDBUFFER_H
#define SLAG_IDX12COMMANDBUFFER_H

#include "../../CommandBuffer.h"
#include <directx/d3d12.h>


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

            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)override;
            void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)override;

            //DX12 Specific commands
        protected:
            void move(IDX12CommandBuffer& from);
            GpuQueue::QueueType _commandType = GpuQueue::Graphics;
            ID3D12GraphicsCommandList7* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
        };

    } // dx
} // slag

#endif //SLAG_IDX12COMMANDBUFFER_H
