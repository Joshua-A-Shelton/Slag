#ifndef SLAG_DX12QUEUE_H
#define SLAG_DX12QUEUE_H
#include "../../GpuQueue.h"
#include <directx/d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12Semaphore;
        class DX12Queue: public GpuQueue
        {
        public:
            DX12Queue(ID3D12CommandQueue* queue,GpuQueue::QueueType type);
            ~DX12Queue()override;
            DX12Queue(const DX12Queue&)=delete;
            DX12Queue& operator=(DX12Queue&)=delete;
            DX12Queue(DX12Queue&& from);
            DX12Queue& operator=(DX12Queue&& from);

            void submit(CommandBuffer* commands)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)override;
            void submit(CommandBuffer* commands, SemaphoreValue& signalFinished)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished,bool forceDependency)override;
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount)override;

            //Frame Submits
            void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount, Frame* presentFrame)override;

            QueueType type()override;
            ID3D12CommandQueue* underlyingQueue();
            void signal(DX12Semaphore* semaphore, uint64_t value);
        private:
            void move(DX12Queue&& from);
            ID3D12CommandQueue* _queue = nullptr;
            GpuQueue::QueueType _type = GpuQueue::QueueType::GRAPHICS;
        };

    } // dx
} // slag

#endif //SLAG_DX12QUEUE_H
