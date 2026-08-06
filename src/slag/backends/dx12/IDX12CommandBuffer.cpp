#include "IDX12CommandBuffer.h"

#include <bit>

#include "DX12Backend.h"
#include "DX12Buffer.h"
#include "DX12GraphicsCard.h"
#include "DX12Texture.h"
#include "slag/exceptions/NotImplemented.h"
#include "slag/utilities/SLAG_ASSERT.h"
#include <directx/d3dx12.h>

#include "DX12ResourceDescriptorHeap.h"
#include "DX12SamplerDescriptorHeap.h"
#include "DX12ShaderPipeline.h"

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
            if (_queueType == QueueType::GRAPHICS)
            {
                //TODO: I don't know if this is the right place for this, I don't know why it has to be set at all because the shader pipeline also has this information
                _commandBuffer->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            }

        }

        void IDX12CommandBuffer::end()
        {
            _commandBuffer->Close();
#ifdef SLAG_DEBUG
            _setViewport = false;
            _setScissor = false;
            _boundPipelineType = BoundPipeLineType::NONE;
            _heapsBound = false;
#endif
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

                bufferBarrier.LayoutBefore = DX12Backend::nativeImageLayout(copyBarrier.layoutBefore);
                bufferBarrier.LayoutAfter = DX12Backend::nativeImageLayout(copyBarrier.layoutAfter);
                bufferBarrier.Subresources = D3D12_BARRIER_SUBRESOURCE_RANGE
                {
                    .IndexOrFirstMipLevel = copyBarrier.baseMipLevel,
                    .NumMipLevels = copyBarrier.mipCount == 0 ? copyBarrier.texture->mipLevels() - copyBarrier.baseMipLevel : copyBarrier.mipCount,
                    .FirstArraySlice = copyBarrier.baseLayer,
                    .NumArraySlices = copyBarrier.layerCount == 0 ? copyBarrier.texture->layers() - copyBarrier.baseLayer : copyBarrier.layerCount,
                    .FirstPlane = 0,
                    .NumPlanes = static_cast<UINT>(std::popcount(static_cast<uint8_t>(Pixel::aspectFlags(copyBarrier.texture->format()))))
                };
                if (copyBarrier.layoutBefore == TextureLayout::UNKNOWN)
                {
                    bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
                    bufferBarrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
                }
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
                if (copyBarrier.layoutBefore == TextureLayout::UNKNOWN)
                {
                    bufferBarrier.AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
                    bufferBarrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_DISCARD;
                }
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

        void IDX12CommandBuffer::bindDescriptorHeaps(ResourceDescriptorHeap* resourceHeap, SamplerDescriptorHeap* samplerHeap)
        {
            SLAG_ASSERT(_queueType != QueueType::TRANSFER && "Command Buffer cannot record commands outside it's capabilities");

            DX12ResourceDescriptorHeap* dx12ResourceHeap = static_cast<DX12ResourceDescriptorHeap*>(resourceHeap);
            DX12SamplerDescriptorHeap* dx12SamplerHeap = static_cast<DX12SamplerDescriptorHeap*>(samplerHeap);
            ID3D12DescriptorHeap* heaps[]{dx12ResourceHeap->dx12Handle(),dx12SamplerHeap->dx12Handle()};
            _commandBuffer->SetDescriptorHeaps(2,heaps);

            if (_queueType == QueueType::GRAPHICS)
            {
                _commandBuffer->SetGraphicsRootSignature(_graphicsCard->rootSignature());
            }
            _commandBuffer->SetComputeRootSignature(_graphicsCard->rootSignature());
#ifdef SLAG_DEBUG
            _heapsBound = true;
#endif

        }

        void IDX12CommandBuffer::setGraphicsShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)
        {

            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");
            SLAG_ASSERT(shaderDataOffset + dataSize < 128 && "Exceeded size of shader parameter data");
            SLAG_ASSERT(shaderDataOffset %4 == 0 && "Shader data offset must be aligned to 4 bytes");
            SLAG_ASSERT(dataSize % 4 == 0 && "dataSize must be multiple of 4");
#ifdef SLAG_DEBUG
            SLAG_ASSERT(_heapsBound && "Heaps must be bound before setting shader parameters");
#endif
            _commandBuffer->SetGraphicsRoot32BitConstants(0,dataSize/4,data,shaderDataOffset/4);
        }

        void IDX12CommandBuffer::setComputeShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)
        {

            SLAG_ASSERT(_queueType != QueueType::TRANSFER && "Command Buffer cannot record commands outside it's capabilities");
            SLAG_ASSERT(shaderDataOffset + dataSize < 128 && "Exceeded size of shader parameter data");
            SLAG_ASSERT(shaderDataOffset %4 == 0 && "Shader data offset must be aligned to 4 bytes");
            SLAG_ASSERT(dataSize % 4 == 0 && "dataSize must be multiple of 4");
#ifdef SLAG_DEBUG
            SLAG_ASSERT(_heapsBound && "Heaps must be bound before setting shader parameters");
#endif
            _commandBuffer->SetComputeRoot32BitConstants(0,dataSize/4,data,shaderDataOffset/4);
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
                    auto subResourceIndex = D3D12CalcSubresource(mapping.subresource.mipLevel,mapping.subresource.baseArrayLayer+layerOffsetIndex,plane,source->mipLevels(),source->layers());
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

        void IDX12CommandBuffer::bindShaderPipeline(ShaderPipeline* pipeline)
        {
            SLAG_ASSERT(_queueType != QueueType::TRANSFER && "Command Buffer cannot record commands outside it's capabilities");

            DX12ShaderPipeline* dx12Pipeline = static_cast<DX12ShaderPipeline*>(pipeline);
            _commandBuffer->SetPipelineState(dx12Pipeline->dx12Handle());
#ifdef SLAG_DEBUG
            if (dx12Pipeline->type() == ShaderPipelineType::COMPUTE)
            {
                _boundPipelineType = BoundPipeLineType::COMPUTE;
            }
            else
            {
                SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

                _boundPipelineType = BoundPipeLineType::GRAPHICS;
            }
#endif
        }

        void IDX12CommandBuffer::beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,
                                                Attachment* depthAttachment, const Rectangle& bounds)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            std::vector<D3D12_RENDER_PASS_RENDER_TARGET_DESC> targets(colorAttachmentCount);
            for (auto i=0; i<colorAttachmentCount; i++)
            {
                SLAG_ASSERT((bool)(colorAttachments[i].texture->usage() & TextureUsageFlags::COLOR_TARGET) && "Not all color attachments are color target textures");
                auto& colorAttachment = colorAttachments[i];
                auto& renderTarget = targets[i];
                renderTarget.BeginningAccess.Type = colorAttachment.autoClear ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                renderTarget.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                D3D12_CLEAR_VALUE clearColor;
                clearColor.Format = DX12Backend::nativeFormat(colorAttachment.texture->format());
                //TODO: I'm not sure this is correct.... colorAttachment.clearValue.color doesn't have to have floats....
                clearColor.Color[0] = colorAttachment.clearValue.color.floats[0];
                clearColor.Color[1] = colorAttachment.clearValue.color.floats[1];
                clearColor.Color[2] = colorAttachment.clearValue.color.floats[2];
                clearColor.Color[3] = colorAttachment.clearValue.color.floats[3];
                renderTarget.BeginningAccess.Clear.ClearValue = clearColor;

                renderTarget.cpuDescriptor = static_cast<DX12Texture*>(colorAttachment.texture)->targetHandle();
            }
            if (depthAttachment == nullptr)
            {
                _commandBuffer->BeginRenderPass(colorAttachmentCount, targets.data(),nullptr,D3D12_RENDER_PASS_FLAG_NONE);
            }
            else
            {
                SLAG_ASSERT((bool)(depthAttachment->texture->usage() & TextureUsageFlags::DEPTH_STENCIL_TARGET) && "Depth/Stencil Attachment must be a depth/stencil target texture");

                D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthTarget{};
                depthTarget.DepthBeginningAccess.Type = depthAttachment->autoClear ? D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR : D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
                depthTarget.DepthBeginningAccess.Clear.ClearValue.Format = DX12Backend::nativeFormat(depthAttachment->texture->format());
                depthTarget.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth = depthAttachment->clearValue.depthStencil.depth;
                depthTarget.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Stencil = depthAttachment->clearValue.depthStencil.stencil;
                depthTarget.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
                if ((bool)(Pixel::aspectFlags(depthAttachment->texture->format()) & PixelAspectFlags::STENCIL_FLAG))
                {
                    depthTarget.StencilBeginningAccess = depthTarget.DepthBeginningAccess;
                    depthTarget.StencilEndingAccess = depthTarget.DepthEndingAccess;
                }
                else
                {
                    depthTarget.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
                    depthTarget.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
                }

                depthTarget.cpuDescriptor = static_cast<DX12Texture*>(depthAttachment->texture)->targetHandle();

                _commandBuffer->BeginRenderPass(colorAttachmentCount, targets.data(), &depthTarget, D3D12_RENDER_PASS_FLAG_NONE);
            }
#ifdef SLAG_DEBUG
            _inRenderPass = true;
#endif

        }

        void IDX12CommandBuffer::endRendering()
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            _commandBuffer->EndRenderPass();
#ifdef SLAG_DEBUG
            _inRenderPass = false;
#endif
        }

        void IDX12CommandBuffer::setViewPort(float x, float y, float width, float height, float minDepth,
            float maxDepth)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            CD3DX12_VIEWPORT vp(x,y,width,height,minDepth,maxDepth);
            _commandBuffer->RSSetViewports(1, &vp);
#if SLAG_DEBUG
            _setViewport = true;
#endif
        }

        void IDX12CommandBuffer::setScissors(const Rectangle& rect)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            D3D12_RECT scissorRect;
            scissorRect.left   = rect.offset.x;
            scissorRect.top    = rect.offset.y;
            scissorRect.right  = rect.offset.x + rect.extent.width;
            scissorRect.bottom = rect.offset.y + rect.extent.height;
            _commandBuffer->RSSetScissorRects(1, &scissorRect);
#if SLAG_DEBUG
            _setScissor = true;
#endif
        }

        void IDX12CommandBuffer::bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            D3D12_INDEX_BUFFER_VIEW indexBufferView{};
            indexBufferView.BufferLocation = static_cast<DX12Buffer*>(buffer)->dx12Handle()->GetGPUVirtualAddress() + offset;
            indexBufferView.SizeInBytes = buffer->size() - offset;
            indexBufferView.Format = indexType == IndexBufferType::UINT_16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            _commandBuffer->IASetIndexBuffer(&indexBufferView);
        }

        void IDX12CommandBuffer::bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

            std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews(bufferCount);
            for (auto i=0; i<bufferCount; i++)
            {
                auto& buffer = buffers[i];
                auto& offset = bufferOffsets[i];
                auto& stride = strides[i];
                vertexBufferViews[i].BufferLocation = static_cast<DX12Buffer*>(buffer)->dx12Handle()->GetGPUVirtualAddress() + offset;
                vertexBufferViews[i].SizeInBytes = buffer->size() - offset;
                vertexBufferViews[i].StrideInBytes = stride;
            }
            _commandBuffer->IASetVertexBuffers(firstBinding, bufferCount, vertexBufferViews.data());
        }

        void IDX12CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                                      uint32_t firstInstance)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            _commandBuffer->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        void IDX12CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
            int32_t vertexOffset, uint32_t firstInstance)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            _commandBuffer->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

        void IDX12CommandBuffer::drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            auto dx12Buffer = static_cast<DX12Buffer*>(buffer);
            _commandBuffer->ExecuteIndirect(_graphicsCard->drawIndirectCommandSignature(), drawCount, dx12Buffer->dx12Handle(), offset,nullptr,0);
        }

        void IDX12CommandBuffer::drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            auto dx12Buffer = static_cast<DX12Buffer*>(buffer);
            _commandBuffer->ExecuteIndirect(_graphicsCard->drawIndexedIndirectCommandSignature(), drawCount, dx12Buffer->dx12Handle(), offset,nullptr,0);

        }

        void IDX12CommandBuffer::drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
                                                   uint64_t countBufferOffset, uint32_t maxDrawCount)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            auto dx12Buffer = static_cast<DX12Buffer*>(buffer);
            auto countBufferDX12 = static_cast<DX12Buffer*>(countBuffer);
            _commandBuffer->ExecuteIndirect(_graphicsCard->drawIndirectCommandSignature(), maxDrawCount, dx12Buffer->dx12Handle(), offset,countBufferDX12->dx12Handle(),countBufferOffset);
        }

        void IDX12CommandBuffer::drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
                                                          uint64_t countBufferOffset, uint32_t maxDrawCount)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_inRenderPass && "Must be in render pass (between beginRendering() and endRendering()) to draw");
            SLAG_ASSERT(_setViewport && "Viewport must be set prior to issuing drawing commands");
            SLAG_ASSERT(_setScissor && "Scissor must be set prior to issuing drawing commands");
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::GRAPHICS && "Must bind graphics pipeline prior to drawing");
#endif
            auto dx12Buffer = static_cast<DX12Buffer*>(buffer);
            auto countBufferDX12 = static_cast<DX12Buffer*>(countBuffer);
            _commandBuffer->ExecuteIndirect(_graphicsCard->drawIndexedIndirectCommandSignature(), maxDrawCount, dx12Buffer->dx12Handle(), offset,countBufferDX12->dx12Handle(),countBufferOffset);
        }

        void IDX12CommandBuffer::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
        {
            SLAG_ASSERT(_queueType != QueueType::TRANSFER && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::COMPUTE && "Must bind compute pipeline prior to dispatching");
            SLAG_ASSERT(!_inRenderPass && "Cannot dispatch within render pass (between beginRendering() and endRendering())");

#endif
            _commandBuffer->Dispatch(groupCountX, groupCountY, groupCountZ);
        }

        void IDX12CommandBuffer::dispatchIndirect(Buffer* buffer, uint64_t offset)
        {
            SLAG_ASSERT(_queueType != QueueType::TRANSFER && "Command Buffer cannot record commands outside it's capabilities");

#if SLAG_DEBUG
            SLAG_ASSERT(_boundPipelineType == BoundPipeLineType::COMPUTE && "Must bind compute pipeline prior to dispatching");
            SLAG_ASSERT(!_inRenderPass && "Cannot dispatch within render pass (between beginRendering() and endRendering())");
#endif
            auto dx12Buffer = static_cast<DX12Buffer*>(buffer);
            _commandBuffer->ExecuteIndirect(_graphicsCard->dispatchIndirectCommandSignature(), 1, dx12Buffer->dx12Handle(), offset,nullptr,0);

        }

        void IDX12CommandBuffer::resolveTexture(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceRect, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Offset2D destinationOffset)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");
            SLAG_ASSERT(source->sampleCount() != SampleCount::ONE && destination->sampleCount() == SampleCount::ONE && "Source texture must be multisampled and destination texture must not be multisampled");
            SLAG_ASSERT(source->format() == destination->format() && "Source and destination textures must have the same format");
            SLAG_ASSERT(source->mipWidth(sourceMip) == destination->mipWidth(destinationMip) && source->mipHeight(sourceMip) == destination->mipHeight(destinationMip) && "Source and destination mips must have the same dimensions");

            DX12Texture* dx12Source = static_cast<DX12Texture*>(source);
            DX12Texture* dx12Destination = static_cast<DX12Texture*>(destination);
            D3D12_RECT dx12SourceRect = {sourceRect.offset.x, sourceRect.offset.y, (long)(sourceRect.offset.x + sourceRect.extent.width), (long)(sourceRect.offset.y + sourceRect.extent.height)};

            auto sourceSubResource = D3D12CalcSubresource(sourceMip,sourceLayer,0,source->mipLevels(),source->layers());
            auto destinationSubResource = D3D12CalcSubresource(destinationMip,destinationLayer,0,destination->mipLevels(),destination->layers());

            _commandBuffer->ResolveSubresourceRegion(
                dx12Destination->dx12Handle(),
                destinationSubResource,
                destinationOffset.x,
                destinationOffset.y,
                dx12Source->dx12Handle(),
                sourceSubResource,
                &dx12SourceRect,
                DX12Backend::nativeFormat(source->format()),
                D3D12_RESOLVE_MODE::D3D12_RESOLVE_MODE_AVERAGE);
        }

        void IDX12CommandBuffer::copyTextureRegion(PixelAspect aspect, Texture* source, uint32_t sourceLayer, uint32_t sourceMip,
            Rectangle sourceRect, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,
            Offset2D destinationOffset)
        {
            SLAG_ASSERT(_queueType == QueueType::GRAPHICS && "Command Buffer cannot record commands outside it's capabilities");
            auto src = static_cast<DX12Texture*>(source);
            auto dst = static_cast<DX12Texture*>(destination);

            auto planeSlice = 0;
            if (aspect == PixelAspect::STENCIL)
            {
                planeSlice = 1;
            }

            D3D12_TEXTURE_COPY_LOCATION copyLocation
            {
                .pResource = src->dx12Handle(),
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = D3D12CalcSubresource(sourceMip,sourceLayer,planeSlice,source->mipLevels(),source->layers()),
            };

            D3D12_TEXTURE_COPY_LOCATION dstCopyLocation
            {
                .pResource = dst->dx12Handle(),
                .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                .SubresourceIndex = D3D12CalcSubresource(destinationMip,destinationLayer,planeSlice,destination->mipLevels(),destination->layers()),
            };

            D3D12_BOX copyRegion =
            {
                .left = static_cast<UINT>(sourceRect.offset.x),
                .top = static_cast<UINT>(sourceRect.offset.y),
                .front = 0,
                .right = sourceRect.offset.x + sourceRect.extent.width,
                .bottom = sourceRect.offset.y + sourceRect.extent.height,
                .back = 1,
            };
            _commandBuffer->CopyTextureRegion(&dstCopyLocation,destinationOffset.x,destinationOffset.y,0, &copyLocation,&copyRegion);
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
