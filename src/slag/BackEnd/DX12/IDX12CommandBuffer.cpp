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
            if(DX12Lib::card()->supportsEnhancedBarriers())
            {
                std::vector<D3D12_BARRIER_GROUP> barrierGroups;
                std::vector<D3D12_TEXTURE_BARRIER> textureBarriers(imageBarrierCount);
                if (imageBarrierCount)
                {
                    D3D12_BARRIER_GROUP imageGroup{};
                    imageGroup.Type = D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_TEXTURE;
                    imageGroup.NumBarriers = imageBarrierCount;
                    for (size_t i = 0; i < imageBarrierCount; i++)
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
                        if (barrier.oldLayout == Texture::Layout::UNDEFINED)
                        {
                            dxBarrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
                        }
                    }
                    barrierGroups.push_back(D3D12_BARRIER_GROUP(D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_TEXTURE, static_cast<UINT32>(imageBarrierCount), {.pTextureBarriers=textureBarriers.data()}));
                }

                if (bufferBarrierCount > 0)
                {
                    std::vector<D3D12_BUFFER_BARRIER> gpuBufferBarriers(bufferBarrierCount);
                    for (size_t i = 0; i < bufferBarrierCount; i++)
                    {
                        auto& barrierDesc = bufferBarriers[i];
                        auto buffer = dynamic_cast<DX12Buffer*>(barrierDesc.buffer);
                        auto& barrier = gpuBufferBarriers[i];
                        barrier.pResource = buffer->underlyingBuffer();
                        barrier.AccessBefore = std::bit_cast<D3D12_BARRIER_ACCESS>(barrierDesc.accessBefore);
                        barrier.AccessAfter = std::bit_cast<D3D12_BARRIER_ACCESS>(barrierDesc.accessAfter);
                        barrier.SyncBefore = std::bit_cast<D3D12_BARRIER_SYNC>(barrierDesc.syncBefore);
                        barrier.SyncAfter = std::bit_cast<D3D12_BARRIER_SYNC>(barrierDesc.syncAfter);
                    }
                    barrierGroups.push_back(D3D12_BARRIER_GROUP(D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_BUFFER, static_cast<UINT32>(bufferBarrierCount), {.pBufferBarriers=gpuBufferBarriers.data()}));
                }


                if (memoryBarrierCount > 0)
                {
                    std::vector<D3D12_GLOBAL_BARRIER> globalBarriers(memoryBarrierCount);
                    for (size_t i = 0; i < memoryBarrierCount; i++)
                    {
                        auto& barrierDesc = memoryBarriers[i];
                        auto barrier = globalBarriers[i];
                        barrier.AccessBefore = std::bit_cast<D3D12_BARRIER_ACCESS>(barrierDesc.accessBefore);
                        barrier.AccessAfter = std::bit_cast<D3D12_BARRIER_ACCESS>(barrierDesc.accessAfter);
                        barrier.SyncBefore = std::bit_cast<D3D12_BARRIER_SYNC>(barrierDesc.syncBefore);
                        barrier.SyncAfter = std::bit_cast<D3D12_BARRIER_SYNC>(barrierDesc.syncAfter);
                    }
                    barrierGroups.push_back(D3D12_BARRIER_GROUP(D3D12_BARRIER_TYPE::D3D12_BARRIER_TYPE_GLOBAL, static_cast<UINT32>(memoryBarrierCount), {.pGlobalBarriers=globalBarriers.data()}));

                }

                _buffer->Barrier(barrierGroups.size(), barrierGroups.data());
            }
            else
            {
                std::vector<D3D12_RESOURCE_BARRIER> barriers;
                for(size_t i=0; i< imageBarrierCount; i++)
                {
                    auto& barrierDesc = imageBarriers[i];
                    auto image = dynamic_cast<DX12Texture*>(barrierDesc.texture);
                    barriers.push_back({.Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,.Transition={.pResource=image->texture(),.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,.StateBefore=DX12Lib::stateLayout(barrierDesc.oldLayout),.StateAfter=DX12Lib::stateLayout(barrierDesc.newLayout)}});
                }
                for(size_t i=0; i< bufferBarrierCount; i++)
                {
                    auto& barrierDesc = bufferBarriers[i];
                    auto buffer = dynamic_cast<DX12Buffer*>(barrierDesc.buffer);
                    barriers.push_back({.Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV,.UAV={.pResource =buffer->underlyingBuffer()}});
                }
                if(memoryBarrierCount)
                {
                    barriers.push_back({.Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV,.UAV={.pResource =nullptr}});
                }
                _buffer->ResourceBarrier(barriers.size(),barriers.data());
            }
        }

        void IDX12CommandBuffer::clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            auto image = dynamic_cast<DX12Texture*>(texture);
            ImageBarrier barrier{.texture=texture,.oldLayout=currentLayout,.newLayout=Texture::RENDER_TARGET,.accessBefore=BarrierAccessFlags::NONE,.accessAfter=BarrierAccessFlags::NONE,.syncBefore=syncBefore,.syncAfter=PipelineStageFlags::NONE};
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
            _buffer->ClearRenderTargetView(image->descriptorHandle(),color.floats,0, nullptr);
            barrier.oldLayout = Texture::RENDER_TARGET;
            barrier.newLayout = endingLayout;
            barrier.accessBefore = BarrierAccessFlags::ALL_READ;
            barrier.accessAfter = BarrierAccessFlags::NONE;
            barrier.syncBefore = PipelineStageFlags::TRANSFER;
            barrier.syncAfter = syncAfter;
            insertBarriers(&barrier,1, nullptr,0, nullptr,0);
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