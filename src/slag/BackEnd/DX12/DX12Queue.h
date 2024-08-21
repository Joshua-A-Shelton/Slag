#ifndef SLAG_DX12QUEUE_H
#define SLAG_DX12QUEUE_H
#include "../../GpuQueue.h"
#include <d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12Queue: public GpuQueue
        {
        public:
            DX12Queue(ID3D12CommandQueue* queue,GpuQueue::QueueType type);
            ~DX12Queue();
            DX12Queue(const DX12Queue&)=delete;
            DX12Queue& operator=(DX12Queue&)=delete;
            DX12Queue(DX12Queue&& from);
            DX12Queue& operator=(DX12Queue&& from);

            void submit(CommandBuffer* commands)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)override;
            void submit(CommandBuffer* commands, SemaphoreValue& signalFinished)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished,bool forceDependency)override;
            QueueType type()override;
        private:
            void move(DX12Queue&& from);
            ID3D12CommandQueue* _queue = nullptr;
            GpuQueue::QueueType _type = GpuQueue::QueueType::Graphics;
        };

    } // dx
} // slag

#endif //SLAG_DX12QUEUE_H
