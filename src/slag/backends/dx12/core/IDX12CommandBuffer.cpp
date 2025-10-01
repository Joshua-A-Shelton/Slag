#include "IDX12CommandBuffer.h"

#include "DX12Buffer.h"
#include "DX12Texture.h"
#include "slag/backends/dx12/DX12Backend.h"
#include <directx/d3dx12.h>

#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {

        GPUQueue::QueueType IDX12CommandBuffer::commandType()
        {
            return _queueType;
        }

        void IDX12CommandBuffer::begin()
        {
            _buffer->Reset(_pool,nullptr);
        }

        void IDX12CommandBuffer::end()
        {
            _buffer->Close();
        }

        void IDX12CommandBuffer::insertBarriers(TextureBarrier* textureBarriers, uint32_t textureBarrierCount,
                                                BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount, GlobalBarrier* memoryBarriers,
                                                uint32_t memoryBarrierCount)
        {
            throw std::runtime_error("IDX12CommandBuffer::insertBarriers(): not implemented");
        }

        void IDX12CommandBuffer::insertBarrier(const TextureBarrier& barrier)
        {
            insertBarriers(const_cast<TextureBarrier*>(&barrier),1,nullptr,0,nullptr,0);
        }

        void IDX12CommandBuffer::insertBarrier(const BufferBarrier& barrier)
        {
            insertBarriers(nullptr,0,const_cast<BufferBarrier*>(&barrier),1,nullptr,0);
        }

        void IDX12CommandBuffer::insertBarrier(const GlobalBarrier& barrier)
        {
            insertBarriers(nullptr,0,nullptr,0,const_cast<GlobalBarrier*>(&barrier),1);
        }

        void IDX12CommandBuffer::clearTexture(Texture* texture, ClearColor color)
        {
            throw std::runtime_error("IDX12CommandBuffer::clearTexture(): not implemented");
        }

        void IDX12CommandBuffer::clearTexture(Texture* texture, ClearDepthStencilValue depthStencil)
        {
            throw std::runtime_error("IDX12CommandBuffer::clearTexture(): not implemented");
        }

        void IDX12CommandBuffer::updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip,
            uint32_t destinationMip)
        {
            throw std::runtime_error("IDX12CommandBuffer::updateMip(): not implemented");
        }

        void IDX12CommandBuffer::copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length,
            Buffer* destination, uint64_t destinationOffset)
        {
            DX12Buffer* src = static_cast<DX12Buffer*>(source);
            DX12Buffer* dst = static_cast<DX12Buffer*>(destination);
            _buffer->CopyBufferRegion(dst->dx12Handle(),destinationOffset,src->dx12Handle(),sourceOffset,length);
        }

        void IDX12CommandBuffer::copyTextureToBuffer(Texture* source, Buffer* destination,TextureBufferMapping* copyData, uint32_t subresourceCount)
        {
            auto tex = static_cast<DX12Texture*>(source);
            auto buf = static_cast<DX12Buffer*>(destination);

            auto format = DX12Backend::dx12Format(tex->format());


            for (auto i = 0; i < subresourceCount; ++i)
            {
                auto& mapping = copyData[i];
                SLAG_ASSERT(std::popcount((uint8_t)mapping.textureSubresource.aspectFlags) == 1 && "Only a single aspect may be specified per subresource");
                auto pixelSize = Pixels::size(tex->format(),mapping.textureSubresource.aspectFlags);

                UINT plane = 0;
                if (mapping.textureSubresource.aspectFlags == Pixels::AspectFlags::STENCIL)
                {
                    plane = 1;
                }

                UINT layerOffset = 0;
                uint32_t layerOffsetIndex = 0;
                for (auto i=0; i<mapping.textureSubresource.layerCount; ++i)
                {
                    auto subResourceIndex = D3D12CalcSubresource(mapping.textureSubresource.mipLevel,mapping.textureSubresource.baseArrayLayer+layerOffsetIndex,plane,tex->mipLevels(),tex->layers());
                    D3D12_TEXTURE_COPY_LOCATION sourceLocation
                    {
                        .pResource = tex->dx12Handle(),
                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                        .SubresourceIndex = subResourceIndex,
                    };

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                    footprint.Offset = mapping.bufferOffset + layerOffset;
                    footprint.Footprint.Format = format;
                    footprint.Footprint.Width = mapping.textureExtent.width;
                    footprint.Footprint.Height = mapping.textureExtent.height;
                    footprint.Footprint.Depth = mapping.textureExtent.depth;
                    footprint.Footprint.RowPitch = footprint.Footprint.Width * (pixelSize);//tightly packed



                    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(buf->dx12Handle(),footprint);
                    D3D12_BOX srcBox{};
                    srcBox.left=mapping.textureOffset.x;
                    srcBox.top=mapping.textureOffset.y;
                    srcBox.front=mapping.textureOffset.z;
                    srcBox.right=srcBox.left+mapping.textureExtent.width;
                    srcBox.bottom=srcBox.top+mapping.textureExtent.height;
                    srcBox.back=srcBox.front+mapping.textureExtent.depth;

                    _buffer->CopyTextureRegion(&dst,0,0,0,&sourceLocation, &srcBox);
                    layerOffsetIndex++;
                    layerOffset+= mapping.textureExtent.width * mapping.textureExtent.height * mapping.textureExtent.depth * pixelSize;
                }

            }
        }

        void IDX12CommandBuffer::copyBufferToTexture(Buffer* source, Texture* destination,TextureBufferMapping* mappings, uint32_t mappingCount)
        {
            auto tex = static_cast<DX12Texture*>(destination);
            auto buf = static_cast<DX12Buffer*>(source);

            auto format = DX12Backend::dx12Format(tex->format());

            for (auto i = 0; i < mappingCount; ++i)
            {
                auto& mapping = mappings[i];
                SLAG_ASSERT(std::popcount((uint8_t)mapping.textureSubresource.aspectFlags) == 1 && "Only a single aspect may be specified per subresource");
                auto pixelSize = Pixels::size(tex->format(),mapping.textureSubresource.aspectFlags);
                UINT plane = 0;
                if (mapping.textureSubresource.aspectFlags == Pixels::AspectFlags::STENCIL)
                {
                    plane = 1;
                }

                UINT layerOffset = 0;
                uint32_t layerOffsetIndex = 0;
                for (auto i=0; i<mapping.textureSubresource.layerCount; ++i)
                {
                    auto subResourceIndex = D3D12CalcSubresource(mapping.textureSubresource.mipLevel,mapping.textureSubresource.baseArrayLayer+layerOffsetIndex,plane,tex->mipLevels(),tex->layers());
                    D3D12_TEXTURE_COPY_LOCATION destinationLocation
                    {
                        .pResource = tex->dx12Handle(),
                        .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                        .SubresourceIndex = subResourceIndex,
                    };

                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                    footprint.Offset = mapping.bufferOffset + layerOffset;
                    footprint.Footprint.Format = format;
                    footprint.Footprint.Width = mapping.textureExtent.width;
                    footprint.Footprint.Height = mapping.textureExtent.height;
                    footprint.Footprint.Depth = mapping.textureExtent.depth;
                    footprint.Footprint.RowPitch = footprint.Footprint.Width * (pixelSize);//tightly packed

                    auto src = CD3DX12_TEXTURE_COPY_LOCATION(buf->dx12Handle(),footprint);

                    _buffer->CopyTextureRegion(&destinationLocation,mapping.textureOffset.x,mapping.textureOffset.y,mapping.textureOffset.z,&src, nullptr);
                    layerOffsetIndex++;
                    layerOffset+= mapping.textureExtent.width * mapping.textureExtent.height * mapping.textureExtent.depth * pixelSize;
                }

            }
        }

        void IDX12CommandBuffer::blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,
            Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea)
        {
            throw std::runtime_error("IDX12CommandBuffer::blit(): not implemented");
        }

        void IDX12CommandBuffer::blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,
            Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea,
            Pixels::AspectFlags aspects)
        {
            throw std::runtime_error("IDX12CommandBuffer::blit(): not implemented");
        }

        void IDX12CommandBuffer::resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset,
            Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Offset destinationOffset,
            Extent resolveExtent)
        {
            throw std::runtime_error("IDX12CommandBuffer::resolve(): not implemented");
        }

        void IDX12CommandBuffer::fillBuffer(Buffer* buffer, uint64_t offset, uint64_t length, uint32_t data)
        {
            auto buf = static_cast<DX12Buffer*>(buffer);
            std::vector<D3D12_WRITEBUFFERIMMEDIATE_PARAMETER> rdata(length/4);
            for(size_t i=0; i< rdata.size(); i++)
            {
                rdata[i].Dest = buf->dx12Handle()->GetGPUVirtualAddress()+offset+(i*4);
                rdata[i].Value=data;
            }
            _buffer->WriteBufferImmediate(rdata.size(),rdata.data(), nullptr);
        }

        void IDX12CommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth,
            float maxDepth)
        {
            throw std::runtime_error("IDX12CommandBuffer::setViewPort(): not implemented");
        }

        void IDX12CommandBuffer::setScissors(Rectangle rectangle)
        {
            throw std::runtime_error("IDX12CommandBuffer::setScissors(): not implemented");
        }

        void IDX12CommandBuffer::setBlendConstants(float r, float g, float b, float a)
        {
            throw std::runtime_error("IDX12CommandBuffer::setBlendConstants(): not implemented");
        }

        void IDX12CommandBuffer::setStencilReference(uint32_t reference)
        {
            throw std::runtime_error("IDX12CommandBuffer::setStencilReference(): not implemented");
        }

        void IDX12CommandBuffer::beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,
                                                Attachment* depthAttachment, Rectangle bounds)
        {
            throw std::runtime_error("IDX12CommandBuffer::beginRendering(): not implemented");
        }

        void IDX12CommandBuffer::endRendering()
        {
            throw std::runtime_error("IDX12CommandBuffer::endRendering(): not implemented");
        }

        void IDX12CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
            uint32_t firstInstance)
        {
            throw std::runtime_error("IDX12CommandBuffer::draw(): not implemented");
        }

        void IDX12CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            int32_t vertexOffset, uint32_t firstInstance)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexed(): not implemented");
        }

        void IDX12CommandBuffer::drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount,
            uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexedIndirect(): not implemented");
        }

        void IDX12CommandBuffer::drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
            uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndexedIndirect(): not implemented");
        }

        void IDX12CommandBuffer::drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndirect(): not implemented");
        }

        void IDX12CommandBuffer::drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
            uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
        {
            throw std::runtime_error("IDX12CommandBuffer::drawIndirectCount(): not implemented");
        }

        void IDX12CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatch(): not implemented");
        }

        void IDX12CommandBuffer::dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
            uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatchBase(): not implemented");
        }

        void IDX12CommandBuffer::dispatchIndirect(Buffer* buffer, uint64_t offset)
        {
            throw std::runtime_error("IDX12CommandBuffer::dispatchIndirect(): not implemented");
        }

        void IDX12CommandBuffer::bindGraphicsShaderPipeline(ShaderPipeline* pipeline)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindGraphicsShaderPipeline(): not implemented");
        }

        void IDX12CommandBuffer::bindComputeShaderPipeline(ShaderPipeline* pipeline)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindComputeShaderPipeline(): not implemented");
        }

        void IDX12CommandBuffer::bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindGraphicsDescriptorBundle(): not implemented");
        }

        void IDX12CommandBuffer::bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindComputeDescriptorBundle(): not implemented");
        }

        void IDX12CommandBuffer::bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindIndexBuffer(): not implemented");
        }

        void IDX12CommandBuffer::bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers,
                                                   uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)
        {
            throw std::runtime_error("IDX12CommandBuffer::bindVertexBuffers(): not implemented");
        }

        ID3D12GraphicsCommandList7* IDX12CommandBuffer::dx12Handle()
        {
            return _buffer;
        }

        void IDX12CommandBuffer::move(IDX12CommandBuffer& from)
        {
            std::swap(_queueType,from._queueType);
            std::swap( _buffer,from._buffer);
            std::swap( _pool,from._pool);
        }
    } // dx12
} // slag
