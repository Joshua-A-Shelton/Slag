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
