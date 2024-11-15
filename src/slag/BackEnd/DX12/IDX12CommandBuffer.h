#ifndef SLAG_IDX12COMMANDBUFFER_H
#define SLAG_IDX12COMMANDBUFFER_H

#include "../../CommandBuffer.h"
#include <directx/d3d12.h>


namespace slag
{
    namespace dx
    {
        class DX12Semaphore;
        class IDX12CommandBuffer: public CommandBuffer
        {
        public:
            IDX12CommandBuffer();
            virtual ~IDX12CommandBuffer()=default;
            GpuQueue::QueueType commandType()override;
            void end()override;

            void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)override;
            void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;
            void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)override;

            void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)override;
            void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)override;
            void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)override;
            //DX12 Specific commands
            ID3D12GraphicsCommandList7* underlyingCommandBuffer();
        protected:
            void move(IDX12CommandBuffer& from);
            void insertBarriersEnhanced(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount);
            void insertBarriersLegacy(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount);
            GpuQueue::QueueType _commandType = GpuQueue::Graphics;
            ID3D12GraphicsCommandList7* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
            void (IDX12CommandBuffer::*_insertBarriers_ptr)(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)= nullptr;
        };

    } // dx
} // slag

#endif //SLAG_IDX12COMMANDBUFFER_H
