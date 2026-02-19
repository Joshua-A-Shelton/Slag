#include "DX12CommandBuffer.h"

#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        DX12CommandBuffer::DX12CommandBuffer(DX12GraphicsCard* graphicsCard, QueueType type, CommandBufferLevel level)
        {
            _graphicsCard = graphicsCard;
            _queueType = type;
            _level = level;

            D3D12_COMMAND_LIST_TYPE comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            switch (_queueType)
            {
            case QueueType::GRAPHICS:
                comType = D3D12_COMMAND_LIST_TYPE_DIRECT;
                break;
            case QueueType::COMPUTE:
                comType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                break;
            case QueueType::TRANSFER:
                comType = D3D12_COMMAND_LIST_TYPE_COPY;
                break;
            }
            _graphicsCard->device()->CreateCommandAllocator(comType, IID_PPV_ARGS(&_commandPool));
            _graphicsCard->device()->CreateCommandList(0,comType,_commandPool, nullptr, IID_PPV_ARGS(&_commandBuffer));
            _commandBuffer->Close();
        }

        DX12CommandBuffer::~DX12CommandBuffer()
        {
            if (_commandBuffer != nullptr)
            {
                _commandBuffer->Release();
                _commandPool->Release();
            }
        }

        DX12CommandBuffer::DX12CommandBuffer(DX12CommandBuffer&& from) noexcept
        {
            move(from);
        }

        DX12CommandBuffer& DX12CommandBuffer::operator=(DX12CommandBuffer&& from) noexcept
        {
            move(from);
            return *this;
        }

        void DX12CommandBuffer::move(DX12CommandBuffer& from)
        {
            IDXCBMove(from);
        }
    } // dx12
} // slag
