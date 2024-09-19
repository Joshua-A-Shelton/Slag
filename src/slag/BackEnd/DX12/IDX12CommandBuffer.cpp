#include "IDX12CommandBuffer.h"
#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "DX12Buffer.h"
#include "DX12Lib.h"
#include <directx/d3dx12.h>

namespace slag
{
    namespace dx
    {
        void IDX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        GpuQueue::QueueType IDX12CommandBuffer::commandType()
        {
            return _commandType;
        }

        void IDX12CommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)
        {
            std::vector<D3D12_BARRIER_GROUP> barrierGroups;
            std::vector<D3D12_TEXTURE_BARRIER> textureBarriers(imageBarrierCount);
            if(imageBarrierCount)
            {
                D3D12_BARRIER_GROUP imageGroup{};
                imageGroup.Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_TEXTURE;
                imageGroup.NumBarriers = imageBarrierCount;
                for(int i =0; i< imageBarrierCount; i++)
                {
                    auto& barrier = imageBarriers[i];
                    auto image = dynamic_cast<DX12Texture*>(barrier.texture);
                    auto& dxBarrier = textureBarriers[i];
                    dxBarrier.pResource = image->texture();
                    dxBarrier.LayoutBefore = DX12Lib::layout(barrier.oldLayout);
                    dxBarrier.LayoutAfter = DX12Lib::layout(barrier.newLayout);
                    dxBarrier.AccessBefore = std::bit_cast<D3D12_BARRIER_ACCESS>(barrier.accessBefore);
                    dxBarrier.AccessAfter = std::bit_cast<D3D12_BARRIER_ACCESS>(barrier.accessAfter);
                    dxBarrier.SyncBefore = std::bit_cast<D3D12_BARRIER_SYNC>(barrier.syncBefore);
                    dxBarrier.SyncAfter = std::bit_cast<D3D12_BARRIER_SYNC>(barrier.syncAfter);
                    if(barrier.oldLayout == Texture::Layout::UNDEFINED)
                    {
                        dxBarrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
                    }

                    //barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(image->texture(), DX12Lib::layout(barrier.oldLayout), DX12Lib::layout(barrier.newLayout));
                }
            }

            for(int i=0; i< bufferBarrierCount; i++)
            {
                auto& barrier = bufferBarriers[i];
                auto buffer = dynamic_cast<DX12Buffer*>(barrier.buffer);
                //barriers[imageBarrierCount+i] = CD3DX12_RESOURCE_BARRIER::Transition(buffer->underlyingBuffer(),D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_STATE_GENERIC_READ);
            }
            //_buffer->Barrier()
            throw std::runtime_error("not implemented");
        }

        void IDX12CommandBuffer::clearColorImage(slag::Texture* texture, slag::ClearColor color, Texture::Layout layout)
        {
            throw std::runtime_error("not implemented");
        }

        void IDX12CommandBuffer::copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)
        {
            DX12Buffer* src = dynamic_cast<DX12Buffer*>(source);
            DX12Buffer* dst = dynamic_cast<DX12Buffer*>(destination);
            _buffer->CopyBufferRegion(dst->underlyingBuffer(),destinationOffset,src->underlyingBuffer(),sourceOffset,length);
        }

        void IDX12CommandBuffer::move(IDX12CommandBuffer& from)
        {
            std::swap(_pool,from._pool);
            std::swap(_buffer,from._buffer);
            _commandType = from._commandType;
        }
    } // dx
} // slag