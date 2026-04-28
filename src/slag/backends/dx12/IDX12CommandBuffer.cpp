#include "IDX12CommandBuffer.h"

#include <bit>

#include "DX12Backend.h"
#include "DX12Buffer.h"
#include "DX12GraphicsCard.h"
#include "DX12Texture.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"
#include <directx/d3dx12.h>

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

        void IDX12CommandBuffer::begin()
        {
            _commandBuffer->Reset(_commandPool,nullptr);
        }

        void IDX12CommandBuffer::end()
        {
            _commandBuffer->Close();
        }

        void IDX12CommandBuffer::insertBarriers(GlobalBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");

            std::vector<D3D12_GLOBAL_BARRIER> globalBarriers(barrierCount);
            for (int i=0; i<barrierCount; i++)
            {
                auto& copyBarrier = barriers[i];
                auto& globalBarrier = globalBarriers[i];
                globalBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                globalBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                globalBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                globalBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
            }
            D3D12_BARRIER_GROUP barrierGroup{};
            barrierGroup.NumBarriers = barrierCount;
            barrierGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
            barrierGroup.pGlobalBarriers = globalBarriers.data();
            _commandBuffer->Barrier(1, &barrierGroup);
        }

        void IDX12CommandBuffer::insertBarriers(BufferBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");

            std::vector<D3D12_BUFFER_BARRIER> bufferBarriers(barrierCount);
            for (int i=0; i<barrierCount; i++)
            {
                auto& copyBarrier = barriers[i];
                auto& bufferBarrier = bufferBarriers[i];
                bufferBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                bufferBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                bufferBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                bufferBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
                bufferBarrier.pResource = static_cast<DX12Buffer*>(copyBarrier.buffer)->dx12Handle();
                bufferBarrier.Offset = copyBarrier.offset;
                bufferBarrier.Size = copyBarrier.length == 0 ? copyBarrier.buffer->size()-copyBarrier.offset : copyBarrier.length;
            }
            D3D12_BARRIER_GROUP barrierGroup{};
            barrierGroup.NumBarriers = barrierCount;
            barrierGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
            barrierGroup.pBufferBarriers = bufferBarriers.data();
            _commandBuffer->Barrier(1, &barrierGroup);
        }

        void IDX12CommandBuffer::insertBarriers(TextureBarrier* barriers, uint32_t barrierCount)
        {
            SLAG_ASSERT(barriers != nullptr && "barriers cannot be nullptr");
            SLAG_ASSERT(barrierCount != 0 && "barriersCount cannot be 0");

            std::vector<D3D12_TEXTURE_BARRIER> textureBarriers(barrierCount);
            for (int i=0; i<barrierCount; i++)
            {
                auto& copyBarrier = barriers[i];
                auto& bufferBarrier = textureBarriers[i];
                bufferBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                bufferBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                bufferBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                bufferBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
                bufferBarrier.pResource = static_cast<DX12Texture*>(copyBarrier.texture)->dx12Handle();
                //bufferBarrier.Flags = ;
                bufferBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
                bufferBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
                bufferBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE
                {
                    .IndexOrFirstMipLevel = copyBarrier.baseMipLevel,
                    .NumMipLevels = copyBarrier.mipCount == 0 ? copyBarrier.texture->mipLevels() - copyBarrier.baseMipLevel : copyBarrier.mipCount,
                    .FirstArraySlice = copyBarrier.baseLayer,
                    .NumArraySlices = copyBarrier.layerCount == 0 ? copyBarrier.texture->layers() - copyBarrier.baseLayer : copyBarrier.layerCount,
                    .FirstPlane = 0,
                    .NumPlanes = static_cast<UINT>(std::popcount(static_cast<uint8_t>(Pixel::aspectFlags(copyBarrier.texture->format()))))
                };
            }
            D3D12_BARRIER_GROUP barrierGroup{};
            barrierGroup.NumBarriers = barrierCount;
            barrierGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            barrierGroup.pTextureBarriers = textureBarriers.data();
            _commandBuffer->Barrier(1, &barrierGroup);
        }

        void IDX12CommandBuffer::insertBarriers(GlobalBarrier* globalBarriers, uint32_t globalBarrierCount,
            BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount, TextureBarrier* textureBarriers,
            uint32_t textureBarrierCount)
        {
            std::vector<D3D12_GLOBAL_BARRIER> globalBarriersNative(globalBarrierCount);
            for (int i=0; i<globalBarrierCount; i++)
            {
                auto& copyBarrier = globalBarriers[i];
                auto& globalBarrier = globalBarriersNative[i];
                globalBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                globalBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                globalBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                globalBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
            }

            std::vector<D3D12_BUFFER_BARRIER> bufferBarriersNative(bufferBarrierCount);
            for (int i=0; i<bufferBarrierCount; i++)
            {
                auto& copyBarrier = bufferBarriers[i];
                auto& bufferBarrier = bufferBarriersNative[i];
                bufferBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                bufferBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                bufferBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                bufferBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
                bufferBarrier.pResource = static_cast<DX12Buffer*>(copyBarrier.buffer)->dx12Handle();
                bufferBarrier.Offset = copyBarrier.offset;
                bufferBarrier.Size = copyBarrier.length == 0 ? copyBarrier.buffer->size()-copyBarrier.offset : copyBarrier.length;
            }

            std::vector<D3D12_TEXTURE_BARRIER> textureBarriersNative(textureBarrierCount);
            for (int i=0; i<textureBarrierCount; i++)
            {
                auto& copyBarrier = textureBarriers[i];
                auto& bufferBarrier = textureBarriersNative[i];
                bufferBarrier.AccessBefore = DX12Backend::nativeMemoryCaches(copyBarrier.flush);
                bufferBarrier.AccessAfter = DX12Backend::nativeMemoryCaches(copyBarrier.invalidate);
                bufferBarrier.SyncBefore = DX12Backend::nativePipelineStages(copyBarrier.syncBefore);
                bufferBarrier.SyncAfter = DX12Backend::nativePipelineStages(copyBarrier.syncAfter);
                bufferBarrier.pResource = static_cast<DX12Texture*>(copyBarrier.texture)->dx12Handle();
                //bufferBarrier.Flags = ;
                bufferBarrier.LayoutBefore = D3D12_BARRIER_LAYOUT_COMMON;
                bufferBarrier.LayoutAfter = D3D12_BARRIER_LAYOUT_COMMON;
                bufferBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE
                {
                    .IndexOrFirstMipLevel = copyBarrier.baseMipLevel,
                    .NumMipLevels = copyBarrier.mipCount == 0 ? copyBarrier.texture->mipLevels() - copyBarrier.baseMipLevel : copyBarrier.mipCount,
                    .FirstArraySlice = copyBarrier.baseLayer,
                    .NumArraySlices = copyBarrier.layerCount == 0 ? copyBarrier.texture->layers() - copyBarrier.baseLayer : copyBarrier.layerCount,
                    .FirstPlane = 0,
                    .NumPlanes = static_cast<UINT>(std::popcount(static_cast<uint8_t>(Pixel::aspectFlags(copyBarrier.texture->format()))))
                };
            }
            D3D12_BARRIER_GROUP barrierGroups[3]{};
            auto globalGroup = barrierGroups[0];
            globalGroup.NumBarriers = globalBarrierCount;
            globalGroup.Type = D3D12_BARRIER_TYPE_GLOBAL;
            globalGroup.pGlobalBarriers = globalBarriersNative.data();

            auto bufferGroup = barrierGroups[1];
            bufferGroup.NumBarriers = bufferBarrierCount;
            bufferGroup.Type = D3D12_BARRIER_TYPE_BUFFER;
            globalGroup.pBufferBarriers = bufferBarriersNative.data();

            auto textureGroup = barrierGroups[2];
            textureGroup.NumBarriers = textureBarrierCount;
            textureGroup.Type = D3D12_BARRIER_TYPE_TEXTURE;
            textureGroup.pTextureBarriers = textureBarriersNative.data();

            _commandBuffer->Barrier(3, barrierGroups);
        }

        void IDX12CommandBuffer::bindDescriptorHeaps(DescriptorHeap* resourceHeap, DescriptorHeap* samplerHeap)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::setInputBindingTable(uint32_t byteOffset, uint32_t heapOffset)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::copyBufferToBuffer(
            Buffer* source,
            uint64_t sourceOffset,
            Buffer* destination,
            uint64_t destinationOffset,
            uint64_t length)
        {
            DX12Buffer* src = static_cast<DX12Buffer*>(source);
            DX12Buffer* dst = static_cast<DX12Buffer*>(destination);
            _commandBuffer->CopyBufferRegion(dst->dx12Handle(),destinationOffset,src->dx12Handle(),sourceOffset,length);
        }

        void IDX12CommandBuffer::copyTextureToBuffer(Texture* source, Buffer* destination,
            TextureBufferMapping* copyData, uint32_t mappingCount)
        {
            auto tex = static_cast<DX12Texture*>(source);
            auto buf = static_cast<DX12Buffer*>(destination);

            auto format = DX12Backend::nativeFormat(tex->format());


            for (auto i = 0; i < mappingCount; ++i)
            {
                auto& mapping = copyData[i];
                auto pixelSize = Pixel::aspectSize(tex->format(),mapping.subresource.aspect);

                UINT plane = 0;
                if (mapping.subresource.aspect == PixelAspect::STENCIL)
                {
                    plane = 1;
                }

                UINT layerOffset = 0;
                uint32_t layerOffsetIndex = 0;
                for (auto i=0; i<mapping.subresource.layerCount; ++i)
                {
                    auto subResourceIndex = D3D12CalcSubresource(mapping.subresource.mipLevel,mapping.subresource.baseArrayLayer+layerOffsetIndex,plane,tex->mipLevels(),tex->layers());
                    D3D12_TEXTURE_COPY_LOCATION sourceLocation
                    {
                        .pResource = tex->dx12Handle(),
                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                        .SubresourceIndex = subResourceIndex,
                    };

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                    footprint.Offset = mapping.bufferOffset + layerOffset;
                    footprint.Footprint.Format = format;
                    footprint.Footprint.Width = mapping.extent.width;
                    footprint.Footprint.Height = mapping.extent.height;
                    footprint.Footprint.Depth = mapping.extent.depth;
                    footprint.Footprint.RowPitch = footprint.Footprint.Width * (pixelSize);//tightly packed



                    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(buf->dx12Handle(),footprint);
                    D3D12_BOX srcBox{};
                    srcBox.left=mapping.offset.x;
                    srcBox.top=mapping.offset.y;
                    srcBox.front=mapping.offset.z;
                    srcBox.right=srcBox.left+mapping.extent.width;
                    srcBox.bottom=srcBox.top+mapping.extent.height;
                    srcBox.back=srcBox.front+mapping.extent.depth;

                    _commandBuffer->CopyTextureRegion(&dst,0,0,0,&sourceLocation, &srcBox);
                    layerOffsetIndex++;
                    layerOffset+= mapping.extent.width * mapping.extent.height * mapping.extent.depth * pixelSize;
                }

            }
        }

        void IDX12CommandBuffer::copyBufferToTexture(Buffer* source, Texture* destination,
            TextureBufferMapping* copyData, uint32_t mappingCount)
        {
            auto tex = static_cast<DX12Texture*>(destination);
            auto buf = static_cast<DX12Buffer*>(source);

            auto format = DX12Backend::nativeFormat(tex->format());

            for (auto i = 0; i < mappingCount; ++i)
            {
                auto& mapping = copyData[i];
                auto pixelSize = Pixel::aspectSize(tex->format(),mapping.subresource.aspect);
                UINT plane = 0;
                if (mapping.subresource.aspect == PixelAspect::STENCIL)
                {
                    plane = 1;
                }

                UINT layerOffset = 0;
                uint32_t layerOffsetIndex = 0;
                for (auto i=0; i<mapping.subresource.layerCount; ++i)
                {
                    auto subResourceIndex = D3D12CalcSubresource(mapping.subresource.mipLevel,mapping.subresource.baseArrayLayer+layerOffsetIndex,plane,tex->mipLevels(),tex->layers());
                    D3D12_TEXTURE_COPY_LOCATION destinationLocation
                    {
                        .pResource = tex->dx12Handle(),
                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                        .SubresourceIndex = subResourceIndex,
                    };

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                    footprint.Offset = mapping.bufferOffset + layerOffset;
                    footprint.Footprint.Format = format;
                    footprint.Footprint.Width = mapping.extent.width;
                    footprint.Footprint.Height = mapping.extent.height;
                    footprint.Footprint.Depth = mapping.extent.depth;
                    footprint.Footprint.RowPitch = footprint.Footprint.Width * (pixelSize);//tightly packed

                    auto src = CD3DX12_TEXTURE_COPY_LOCATION(buf->dx12Handle(),footprint);

                    _commandBuffer->CopyTextureRegion(&destinationLocation,mapping.offset.x,mapping.offset.y,mapping.offset.z,&src, nullptr);
                    layerOffsetIndex++;
                    layerOffset+= mapping.extent.width * mapping.extent.height * mapping.extent.depth * pixelSize;
                }

            }
        }

        void IDX12CommandBuffer::bindGraphicsPipeline(ShaderPipeline* pipeline)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,
            Attachment* depthAttachment, const Rectangle& bounds)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::endRendering()
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth,
            float maxDepth)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::setScissors(const Rectangle& rect)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                      uint32_t firstInstance)
        {
            throw NotImplemented();
        }

        void IDX12CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            int32_t vertexOffset, uint32_t firstInstance)
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
        }
    } // dx12
} // slag
