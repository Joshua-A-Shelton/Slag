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
        IDX12CommandBuffer::IDX12CommandBuffer()
        {
            if(DX12Lib::card()->supportsEnhancedBarriers())
            {
                _insertBarriers_ptr = &IDX12CommandBuffer::insertBarriersEnhanced;
            }
            else
            {
                _insertBarriers_ptr = &IDX12CommandBuffer::insertBarriersLegacy;
            }
        }

        void IDX12CommandBuffer::move(IDX12CommandBuffer& from)
        {
            std::swap(_pool,from._pool);
            std::swap(_buffer,from._buffer);
            _commandType = from._commandType;
            _insertBarriers_ptr = from._insertBarriers_ptr;
        }

        ID3D12GraphicsCommandList7* IDX12CommandBuffer::underlyingCommandBuffer()
        {
            return _buffer;
        }

        void IDX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        GpuQueue::QueueType IDX12CommandBuffer::commandType()
        {
            return _commandType;
        }

        void IDX12CommandBuffer::insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers,size_t memoryBarrierCount)
        {
            (this->*(this->_insertBarriers_ptr))(imageBarriers, imageBarrierCount, bufferBarriers, bufferBarrierCount, memoryBarriers,memoryBarrierCount);
        }

        void IDX12CommandBuffer::insertBarriersEnhanced(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)
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
                    dxBarrier.LayoutBefore = DX12Lib::barrierLayout(barrier.oldLayout);
                    dxBarrier.LayoutAfter = DX12Lib::barrierLayout(barrier.newLayout);
                    dxBarrier.AccessBefore = std::bit_cast<D3D12_BARRIER_ACCESS>(barrier.accessBefore);
                    dxBarrier.AccessAfter = std::bit_cast<D3D12_BARRIER_ACCESS>(barrier.accessAfter);
                    dxBarrier.SyncBefore = std::bit_cast<D3D12_BARRIER_SYNC>(barrier.syncBefore);
                    dxBarrier.SyncAfter = std::bit_cast<D3D12_BARRIER_SYNC>(barrier.syncAfter);
                    dxBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE
                    {
                        .IndexOrFirstMipLevel = barrier.baseMipLevel,
                        .NumMipLevels = barrier.mipCount == 0? image->mipLevels()-barrier.baseMipLevel : barrier.mipCount,
                        .FirstArraySlice = barrier.baseLayer,
                        .NumArraySlices = barrier.layerCount == 0? image->layers()-barrier.baseLayer : barrier.layerCount,
                        .FirstPlane = 0,
                        .NumPlanes = D3D12GetFormatPlaneCount(DX12Lib::card()->device(),image->underlyingFormat()) //there's room  for optimization here, I should probably make a lookup table when I first create the card
                    };
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

        void IDX12CommandBuffer::insertBarriersLegacy(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)
        {
            std::vector<D3D12_RESOURCE_BARRIER> barriers;
            for(size_t i=0; i< imageBarrierCount; i++)
            {

                auto& barrierDesc = imageBarriers[i];
                auto image = dynamic_cast<DX12Texture*>(barrierDesc.texture);
                //TODO, not sure about plane slice being 0, especially for depth/stencil
                auto mipCount = barrierDesc.mipCount != 0 ? barrierDesc.mipCount : image->mipLevels() - barrierDesc.baseMipLevel;
                auto layerCount = barrierDesc.layerCount != 0 ? barrierDesc.layerCount : image->layers() - barrierDesc.baseLayer;
                UINT subresource = D3D12CalcSubresource(barrierDesc.baseMipLevel,barrierDesc.baseLayer,0,mipCount,layerCount);


                barriers.push_back(
                {
                    .Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                    .Transition={.pResource=image->texture(),.Subresource=subresource,.StateBefore=DX12Lib::stateLayout(barrierDesc.oldLayout),.StateAfter=DX12Lib::stateLayout(barrierDesc.newLayout)}
                });
                if((barrierDesc.accessBefore | barrierDesc.accessAfter) != BarrierAccessFlags::NONE)
                {
                    barriers.push_back(
                    {
                        .Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV,
                        .UAV={.pResource=image->texture()}
                    });
                }
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

        void IDX12CommandBuffer::updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout,Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            throw std::runtime_error("IDX12CommandBuffer::updateMipChain is not implemented");
        }


        void IDX12CommandBuffer::copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)
        {
            DX12Buffer* src = dynamic_cast<DX12Buffer*>(source);
            DX12Buffer* dst = dynamic_cast<DX12Buffer*>(destination);
            _buffer->CopyBufferRegion(dst->underlyingBuffer(),destinationOffset,src->underlyingBuffer(),sourceOffset,length);
        }

        void IDX12CommandBuffer::copyImageToBuffer(Texture* texture, Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip, Buffer* buffer, size_t bufferOffset)
        {
            auto tex = dynamic_cast<DX12Texture*>(texture);
            auto buf = dynamic_cast<DX12Buffer*>(buffer);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
            footprint.Offset = bufferOffset;
            footprint.Footprint.Format = tex->underlyingFormat();
            footprint.Footprint.Width = tex->width() >> mip;
            footprint.Footprint.Height = tex->height() >> mip;
            footprint.Footprint.Depth = 1;
            footprint.Footprint.RowPitch = footprint.Footprint.Width * (DX12Lib::formatSize(tex->underlyingFormat())); //there may be an alignment issue I need to deal with here

            auto src = CD3DX12_TEXTURE_COPY_LOCATION(tex->texture(),D3D12CalcSubresource(mip,baseLayer,0,1,layerCount));
            auto dst = CD3DX12_TEXTURE_COPY_LOCATION(buf->underlyingBuffer(),footprint);
            _buffer->CopyTextureRegion(&dst,0,0,0,&src, nullptr);
        }

        void IDX12CommandBuffer::copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)
        {
            auto tex = dynamic_cast<DX12Texture*>(destination);
            auto buf = dynamic_cast<DX12Buffer*>(source);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
            footprint.Offset = sourceOffset;
            footprint.Footprint.Format = tex->underlyingFormat();
            footprint.Footprint.Width = tex->width() >> mipLevel;
            footprint.Footprint.Height = tex->height() >> mipLevel;
            footprint.Footprint.Depth = 1;
            footprint.Footprint.RowPitch = footprint.Footprint.Width * (DX12Lib::formatSize(tex->underlyingFormat())); //there may be an alignment issue I need to deal with here

            auto dst = CD3DX12_TEXTURE_COPY_LOCATION(tex->texture(),D3D12CalcSubresource(mipLevel,layer,0,1,1));
            auto src = CD3DX12_TEXTURE_COPY_LOCATION(buf->underlyingBuffer(),footprint);
            _buffer->CopyTextureRegion(&dst,0,0,0,&src, nullptr);

        }

        void IDX12CommandBuffer::blit(Texture* source, Texture::Layout sourceLayout, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea, Sampler::Filter filter)
        {
            throw std::runtime_error("IDX12CommandBuffer::blit is not implemented");
        }

    } // dx
} // slag