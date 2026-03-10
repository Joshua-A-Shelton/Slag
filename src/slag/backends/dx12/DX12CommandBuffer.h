#ifndef SLAG_DX12COMMANDBUFFER_H
#define SLAG_DX12COMMANDBUFFER_H
#include "IDX12CommandBuffer.h"

namespace slag
{
    namespace dx12
    {
        class DX12CommandBuffer: public IDX12CommandBuffer
        {
        public:
            DX12CommandBuffer(DX12GraphicsCard* graphicsCard,QueueType type);
            ~DX12CommandBuffer()override;
            DX12CommandBuffer(const DX12CommandBuffer&) = delete;
            DX12CommandBuffer& operator=(const DX12CommandBuffer&) = delete;
            DX12CommandBuffer(DX12CommandBuffer&& from) noexcept ;
            DX12CommandBuffer& operator=(DX12CommandBuffer&& from) noexcept;

        private:
            void move(DX12CommandBuffer& from);
        };
    } // dx12
} // slag

#endif //SLAG_DX12COMMANDBUFFER_H
