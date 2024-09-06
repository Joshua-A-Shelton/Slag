#ifndef SLAG_DX12COMMANDBUFFER_H
#define SLAG_DX12COMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "../../Resources/ResourceConsumer.h"
#include "DX12Semaphore.h"
#include <d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12CommandBuffer: public CommandBuffer, resources::ResourceConsumer
        {
        public:
            DX12CommandBuffer(GpuQueue::QueueType commandType);
            ~DX12CommandBuffer()override;
            DX12CommandBuffer(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer& operator=(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer(DX12CommandBuffer&& from);
            DX12CommandBuffer& operator=(DX12CommandBuffer&& from);

            void begin()override;
            void end()override;
            void waitUntilFinished()override;
            bool isFinished()override;
            GpuQueue::QueueType commandType()override;
            friend class DX12Queue;

            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)override;

        private:
            void move(DX12CommandBuffer&& from);
            void _waitUntilFinished();
            GpuQueue::QueueType _commandType = GpuQueue::Graphics;
            ID3D12GraphicsCommandList* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
            DX12Semaphore* _finished = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12COMMANDBUFFER_H
