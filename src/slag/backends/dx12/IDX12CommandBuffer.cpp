#include "IDX12CommandBuffer.h"

#include "DX12Buffer.h"
#include "DX12GraphicsCard.h"
#include "slag/exceptions/NotImplemented.h"

namespace slag
{
    namespace dx12
    {
        GraphicsCard* IDX12CommandBuffer::graphicsCard()
        {
            return _graphicsCard;
        }

        QueueType IDX12CommandBuffer::type() const
        {
            return _queueType;
        }

        CommandBufferLevel IDX12CommandBuffer::level() const
        {
            return _level;
        }

        void IDX12CommandBuffer::begin()
        {
            _commandBuffer->Reset(_commandPool,nullptr);
        }

        void IDX12CommandBuffer::end()
        {
            _commandBuffer->Close();
        }

        void IDX12CommandBuffer::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination,
            uint64_t destinationOffset, uint64_t length)
        {
            DX12Buffer* src = static_cast<DX12Buffer*>(source);
            DX12Buffer* dst = static_cast<DX12Buffer*>(destination);
            _commandBuffer->CopyBufferRegion(dst->dx12Handle(),destinationOffset,src->dx12Handle(),sourceOffset,length);
        }

        void IDX12CommandBuffer::copyTextureToBuffer(Texture* source, Buffer* destination,
            TextureBufferMapping* copyData, uint32_t mappingCount)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::copyBufferToTexture(Buffer* source, Texture* destination,
            TextureBufferMapping* copyData, uint32_t mappingCount)
        {
            throw NotImplemented();
        }

        ID3D12GraphicsCommandList7* IDX12CommandBuffer::dx12Handle() const
        {
            return _commandBuffer;
        }

        void IDX12CommandBuffer::IDXCBMove(IDX12CommandBuffer& from)
        {
            std::swap(_commandBuffer, from._commandBuffer);
            std::swap(_commandPool,from._commandPool);
            _graphicsCard = from._graphicsCard;
            _queueType = from._queueType;
            _level = from._level;
        }
    } // dx12
} // slag
