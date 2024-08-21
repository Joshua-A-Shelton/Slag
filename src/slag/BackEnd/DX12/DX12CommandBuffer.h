#ifndef SLAG_DX12COMMANDBUFFER_H
#define SLAG_DX12COMMANDBUFFER_H
#include "../../CommandBuffer.h"
#include "DX12Semaphore.h"
#include <d3d12.h>
namespace slag
{
    namespace dx
    {

        class DX12CommandBuffer: public CommandBuffer
        {
        public:
            DX12CommandBuffer(D3D12_COMMAND_LIST_TYPE type);
            ~DX12CommandBuffer();
            DX12CommandBuffer(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer& operator=(const DX12CommandBuffer&)=delete;
            DX12CommandBuffer(DX12CommandBuffer&& from);
            DX12CommandBuffer& operator=(DX12CommandBuffer&& from);

            void begin()override;
            void end()override;
            void waitUntilFinished()override;
            bool isFinished()override;
            friend class DX12Queue;
        private:
            void move(DX12CommandBuffer&& from);
            void _waitUntilFinished();
            ID3D12GraphicsCommandList* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
            DX12Semaphore* _finished = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12COMMANDBUFFER_H
