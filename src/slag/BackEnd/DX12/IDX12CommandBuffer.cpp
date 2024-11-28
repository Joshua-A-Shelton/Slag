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
                    auto image = static_cast<DX12Texture*>(barrier.texture);
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
                    auto buffer = static_cast<DX12Buffer*>(barrierDesc.buffer);
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
                auto image = static_cast<DX12Texture*>(barrierDesc.texture);
                //TODO, not sure about plane slice being 0, especially for depth/stencil
                auto mipCount = barrierDesc.mipCount != 0 ? barrierDesc.mipCount : image->mipLevels() - barrierDesc.baseMipLevel;
                auto layerCount = barrierDesc.layerCount != 0 ? barrierDesc.layerCount : image->layers() - barrierDesc.baseLayer;
                UINT subresource = D3D12CalcSubresource(barrierDesc.baseMipLevel,barrierDesc.baseLayer,0,mipCount,layerCount);

                auto before = DX12Lib::stateLayout(barrierDesc.oldLayout);
                auto after = DX12Lib::stateLayout(barrierDesc.newLayout);
                if(before != after)
                {
                    barriers.push_back(
                    {
                        .Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
                        .Transition={.pResource=image->texture(),.Subresource=subresource,.StateBefore=before,.StateAfter=after}
                    });
                }
                if(before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
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
                auto buffer = static_cast<DX12Buffer*>(barrierDesc.buffer);
                barriers.push_back({.Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV,.UAV={.pResource =buffer->underlyingBuffer()}});
            }
            if(memoryBarrierCount)
            {
                barriers.push_back({.Type=D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_UAV,.UAV={.pResource =nullptr}});
            }
            if(!barriers.empty())
            {
                _buffer->ResourceBarrier(barriers.size(),barriers.data());
            }

        }

        void IDX12CommandBuffer::clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)
        {
            assert(commandType() == GpuQueue::GRAPHICS && "clearColorImage is a graphics queue only operation");
            auto image = static_cast<DX12Texture*>(texture);
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
            assert(commandType() == GpuQueue::GRAPHICS && "clearColorImage is a graphics queue only operation");
            throw std::runtime_error("IDX12CommandBuffer::updateMipChain is not implemented");
        }


        void IDX12CommandBuffer::copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)
        {
            DX12Buffer* src = static_cast<DX12Buffer*>(source);
            DX12Buffer* dst = static_cast<DX12Buffer*>(destination);
            _buffer->CopyBufferRegion(dst->underlyingBuffer(),destinationOffset,src->underlyingBuffer(),sourceOffset,length);
        }

        void IDX12CommandBuffer::copyImageToBuffer(Texture* texture, Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip, Buffer* buffer, size_t bufferOffset)
        {
            auto tex = static_cast<DX12Texture*>(texture);
            auto buf = static_cast<DX12Buffer*>(buffer);

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
            auto tex = static_cast<DX12Texture*>(destination);
            auto buf = static_cast<DX12Buffer*>(source);

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
            assert(commandType() == GpuQueue::GRAPHICS && "clearColorImage is a graphics queue only operation");
            throw std::runtime_error("IDX12CommandBuffer::blit is not implemented");
        }

        void IDX12CommandBuffer::beginQuery(QueryPool* queryPool, uint32_t query, bool precise)
        {
            throw std::runtime_error("IDX12CommandBuffer::beginQuery is not implemented");
        }

        void IDX12CommandBuffer::beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount, Attachment* depthAttachment,Rectangle bounds)
        {
            throw std::runtime_error("IDX12CommandBuffer::beginRendering is not implemented");
        }

        void IDX12CommandBuffer::bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, size_t offset)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindIndexBuffer is not implemented");
        }

        void IDX12CommandBuffer::bindGraphicsShader(Shader* shader)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindGraphicsShader is not implemented");
        }

        void IDX12CommandBuffer::bindComputeShader(Shader* shader)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindComputeShader is not implemented");
        }

        void IDX12CommandBuffer::bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, size_t* offsets, size_t bindingCount)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindVertexBuffers is not implemented");
        }

        void IDX12CommandBuffer::clearDepthStencilImage(Texture* texture, ClearDepthStencil clear, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore,
                                                        PipelineStages syncAfter)
        {
            throw std::runtime_error("IDX12CommandBuffer::clearDepthStencilImage is not implemented");
        }

        void IDX12CommandBuffer::copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride, QueryPool::ResultFlag flags)
        {
            throw std::runtime_error("IDX12CommandBuffer::copyQueryPoolResults is not implemented");
        }

        void IDX12CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatch is not implemented");
        }

        void IDX12CommandBuffer::dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatchBase is not implemented");
        }

        void IDX12CommandBuffer::dispatchIndirect(Buffer* buffer, size_t offset)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatchIndirect is not implemented");
        }

        void IDX12CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
        {
            throw std::runtime_error("IDX12CommandBuffer::draw is not implemented");
        }

        void IDX12CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexed is not implemented");
        }

        void IDX12CommandBuffer::drawIndexedIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexedIndirect is not implemented");
        }

        void IDX12CommandBuffer::drawIndexedIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexedIndirectCount is not implemented");
        }

        void IDX12CommandBuffer::drawIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndirect is not implemented");
        }

        void IDX12CommandBuffer::drawIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndirectCount is not implemented");
        }

        void IDX12CommandBuffer::endQuery(QueryPool* pool, uint32_t query)
        {
            throw std::runtime_error("IDX12CommandBuffer::endQuery is not implemented");
        }

        void IDX12CommandBuffer::endRendering()
        {
            throw std::runtime_error("IDX12CommandBuffer::endRendering is not implemented");
        }

        void IDX12CommandBuffer::fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)
        {
            throw std::runtime_error("IDX12CommandBuffer::fillBuffer is not implemented");
        }

        void IDX12CommandBuffer::resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)
        {
            throw std::runtime_error("IDX12CommandBuffer::resetQueryPool is not implemented");
        }

    } // dx
} // slag