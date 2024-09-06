#ifndef SLAG_DX12COMMANDBUFFER_H
#define SLAG_DX12COMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "../../Resources/ResourceConsumer.h"
#include "IDX12CommandBuffer.h"
#include "DX12Semaphore.h"
#include <d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12CommandBuffer: public IDX12CommandBuffer, resources::ResourceConsumer
        {
        public:
            DX12CommandBuffer(GpuQueue::QueueType commandType);
            ~DX12CommandBuffer()override;
            DX12CommandBuffer(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer& operator=(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer(DX12CommandBuffer&& from);
            DX12CommandBuffer& operator=(DX12CommandBuffer&& from);

            void begin()override;
            void waitUntilFinished()override;
            bool isFinished()override;

            friend class DX12Queue;

        private:
            void move(DX12CommandBuffer&& from);
            void _waitUntilFinished();
            DX12Semaphore* _finished = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12COMMANDBUFFER_H
