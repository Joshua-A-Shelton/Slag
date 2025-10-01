#include "DX12CommandBuffer.h"

#include "DX12DescriptorPool.h"
#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        DX12CommandBuffer::DX12CommandBuffer(GPUQueue::QueueType type)
        {
            _queueType = type;
            D3D12_COMMAND_LIST_TYPE comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (_queueType)
            {
            case GPUQueue::QueueType::GRAPHICS:
                comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            case GPUQueue::QueueType::TRANSFER:
                comType = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            case GPUQueue::QueueType::COMPUTE:
                comType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;

            }
            DX12GraphicsCard::selected()->device()->CreateCommandAllocator(comType, IID_PPV_ARGS(&_pool));
            DX12GraphicsCard::selected()->device()->CreateCommandList(0,comType,_pool, nullptr, IID_PPV_ARGS(&_buffer));
            _buffer->Close();
        }

        DX12CommandBuffer::~DX12CommandBuffer()
        {
            if (_buffer != nullptr)
            {
                _buffer->Release();
                _pool->Release();
            }
        }

        DX12CommandBuffer::DX12CommandBuffer(DX12CommandBuffer&& from)
        {
            move(from);
        }

        DX12CommandBuffer& DX12CommandBuffer::operator=(DX12CommandBuffer&& from)
        {
            move(from);
            return *this;
        }

        void DX12CommandBuffer::bindDescriptorPool(DescriptorPool* pool)
        {
            ID3D12DescriptorHeap* heaps[2];
            auto descriptorPool = static_cast<DX12DescriptorPool*>(pool);
            heaps[0] = descriptorPool->dx12Handle();
            heaps[1] = DX12GraphicsCard::selected()->samplerHeap();
            _buffer->SetDescriptorHeaps(2,heaps);
        }

        void DX12CommandBuffer::move(DX12CommandBuffer& from)
        {
            IDX12CommandBuffer::move(from);
        }
    } // dx12
} // slag
