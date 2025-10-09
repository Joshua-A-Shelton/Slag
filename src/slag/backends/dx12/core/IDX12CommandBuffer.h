#ifndef SLAG_DX12ICOMMANDBUFFER_H
#define SLAG_DX12ICOMMANDBUFFER_H
#include <slag/Slag.h>
#include <d3d12.h>

namespace slag
{
    namespace dx12
    {
        class IDX12CommandBuffer : public CommandBuffer
        {
        public:
            virtual ~IDX12CommandBuffer()override=default;
            ///The type of commands this command buffer can execute
            virtual GPUQueue::QueueType commandType()override;
            ///Start recording commands
            virtual void begin() override;
            ///End recording commands
            virtual void end() override;


            /**
             * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
             * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
             * @param textureBarrierCount Number of texture barriers
             * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
             * @param bufferBarrierCount Number of buffer barriers
             * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
             * @param memoryBarrierCount Number of memory barriers
             */
            virtual void insertBarriers(TextureBarrier* textureBarriers, uint32_t textureBarrierCount,
                                        BufferBarrier* bufferBarriers, uint32_t bufferBarrierCount,
                                        GlobalBarrier* memoryBarriers, uint32_t memoryBarrierCount) override;
            ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
            virtual void insertBarrier(const TextureBarrier& barrier) override;
            ///Insert GPU execution barrier that ensures an operation on a buffer finishes before the next operation on it can continue
            virtual void insertBarrier(const BufferBarrier& barrier) override;
            ///Insert GPU execution barrier that ensures an operations in the queue finishes before additional operations can continue
            virtual void insertBarrier(const GlobalBarrier& barrier) override;

            /**
             * Clear all sub-resources of a color texture
             * @param texture The texture to clear
             * @param color The color to clear it to
             */
            virtual void clearTexture(Texture* texture, ClearColor color) override;

            /**
             * clear all sub-resources of a depth/stencil texture
             * @param texture The texture to clear
             * @param depthStencil The value to clear it to
             */
            virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil) override;
            /**
             * Propagate an image in a mip chain to a different level in the chain
             * @param texture Texture to update
             * @param layer Texture layer to update
             * @param sourceMip Mip level that acts as a source that is applied to the destination
             * @param destinationMip Mip level to be applied to
             */
            virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip, uint32_t destinationMip) override;
            /**
             * Copy contents of one buffer to another
             * @param source Buffer to copy from
             * @param sourceOffset Offset in the buffer to start copying from
             * @param length Length of data in bytes to copy to the new buffer
             * @param destination Buffer to copy to
             * @param destinationOffset Offset in the buffer to start copying to
             */
            virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination,
                                            uint64_t destinationOffset) override;
            /**
             * Copy texel data of a texture to a buffer
             * @param source Texture to copy from
             * @param destination Buffer to copy to
             * @param copyData Parts of the texture to copy
             * @param subresourceCount Number of items in subresources array
             */
            virtual void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData,
                                             uint32_t subresourceCount) override;

            /**
             * Copy data from a buffer to a texture
             * @param source Buffer containing data to be copied
             * @param destination Texture to copy the data into
             * @param mappings How to map the data from the buffer to the texture
             * @param mappingCount How many mappings have been supplied
             */
            virtual void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* mappings,
                                             uint32_t mappingCount) override;

            /**
             * Draw the contents of one texture into another
             * @param source source The texture to copy from
             * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
             * @param sourceMip sourceMip The mipmap level of the source texture to copy from
             * @param sourceArea The area of the texture to copy (sized to chosen mip level)
             * @param destination The texture to draw to
             * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
             * @param destinationMip The mipmap level of the source texture to draw to
             * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
             */
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,
                              Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,
                              Rectangle destinationArea) override;
            /**
             * Draw the contents of one texture into another
             * @param source source The texture to copy from
             * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
             * @param sourceMip sourceMip The mipmap level of the source texture to copy from
             * @param sourceArea The area of the texture to copy (sized to chosen mip level)
             * @param destination The texture to draw to
             * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
             * @param destinationMip The mipmap level of the source texture to draw to
             * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
             * @param aspects The pixel aspects to copy over from the old image to the new
             */
            virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea,
                              Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,
                              Rectangle destinationArea, Pixels::AspectFlags aspects) override;

            /**
             * Resolve and draw the contents of a multi-sampled image into another
             * @param source The texture to copy from
             * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
             * @param sourceMip The mipmap level of the source texture to copy from
             * @param sourceOffset The offset of the source to get resolve data from
             * @param destination The texture to draw to
             * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
             * @param destinationMip The mipmap level of the source texture to draw to
             * @param destinationOffset The offset of the destination to resolve data to
             * @param resolveExtent The size of the area to get resolve data from and to draw to
             */
            virtual void resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Offset sourceOffset,
                                 Texture* destination, uint32_t destinationLayer, uint32_t destinationMip,
                                 Offset destinationOffset, Extent resolveExtent) override;

            /**
             * Fill a buffer with a default value
             * @param buffer Buffer to fill
             * @param offset Byte offset of buffer to start filling from
             * @param length The length to fill (must be multiple of 4)
             * @param data Value to fill the buffer with (repeating)
             */
            virtual void fillBuffer(Buffer* buffer, uint64_t offset, uint64_t length, uint32_t data) override;

            /**
            * Sets the drawing area relative to the window, final image will be drawn scaled to the drawing area
            * @param x Pixels away from the left of window to draw
            * @param y Pixels away from the top of the window to draw
            * @param width width in pixels of drawing area
            * @param height height in pixels of drawing area
            * @param minDepth minimum depth of drawn pixels
            * @param maxDepth maximum depth of drawn pixels
            */
            virtual void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth) override;

            /**
             * Sets the drawing area relative to the window, final image will be clipped to the drawing area
             * @param rectangle
             */
            virtual void setScissors(Rectangle rectangle) override;
            /**
             *
             * @param r Red constant
             * @param g Green constant
             * @param b Blue constant
             * @param a Alpha constant
             */
            virtual void setBlendConstants(float r, float g, float b, float a) override;
            /**
             * Set the value to compare against in stencil test
             * @param reference
             */
            virtual void setStencilReference(uint32_t reference) override;

            /**
             * Start a renderpass by providing render targets
             * @param colorAttachments Color attachments that shaders will render to in this pass
             * @param colorAttachmentCount Number of Color Attachments
             * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
             * @param bounds Area that is affected in render pass
             */
            virtual void beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,
                                        Attachment* depthAttachment, Rectangle bounds) override;
            /**
             * End a renderpass
             */
            virtual void endRendering() override;

            /**
             * Draw geometry with currently bound vertex buffers and bound shader
             * @param vertexCount Number of vertices per instance
             * @param instanceCount Number of instances
             * @param firstVertex Offset into bound vertex buffer to start drawing from
             * @param firstInstance First instance ID (used in shaders)
             */
            virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                              uint32_t firstInstance) override;

            /**
             * Draw geometry with currently bound vertex buffers, index buffer, and bound shader
             * @param indexCount Number of indexes per instance
             * @param instanceCount Number of instances
             * @param firstIndex Offset into bound buffer to start drawing from
             * @param vertexOffset Offset into bound vertex buffer to start drawing from
             * @param firstInstance First instance ID (used in shaders)
             */
            virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                     int32_t vertexOffset, uint32_t firstInstance) override;

            /**
             * Draw geometry with indirect parameters
             * @param buffer Buffer that contains drawing parameters
             * @param offset Byte offset into buffer where parameters begin
             * @param drawCount Number of draws to exectute, can be zero
             * @param stride Byte stride between successive sets of draw parameters
             */
            virtual void drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;

            /**
             * Draw geometry with indirect parameters, indexed vertices and draw count
             * @param buffer Buffer that contains drawing parameters
             * @param offset Byte offset into buffer where parameters begin
             * @param countBuffer Buffer containing the draw count
             * @param countBufferOffset Byte offset into countBuffer where the draw count begins
             * @param maxDrawCount Maximum number of draws that will be executed
             * @param stride Byte stride between successive sets of draw parameters
             */
            virtual void drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
                                                  uint64_t countBufferOffset, uint32_t maxDrawCount,
                                                  uint32_t stride) override;

            /**
             * Draw geometry with indirect parameters
             * @param buffer Buffer that contains drawing parameters
             * @param offset Byte offset into buffer where parameters begin
             * @param drawCount Number of draws to exectute, can be zero
             * @param stride Byte stride between successive sets of draw parameters
             */
            virtual void drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) override;

            /**
             * Draw geometry with indirect parameters and draw count
             * @param buffer Buffer that contains drawing parameters
             * @param offset Byte offset into buffer where parameters begin
             * @param countBuffer Buffer containing the draw count
             * @param countBufferOffset Byte offset into countBuffer where the draw count begins
             * @param maxDrawCount Maximum number of draws that will be executed
             * @param stride Byte stride between successive sets of draw parameters
             */
            virtual void drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer,
                                           uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride) override;

            /**
             * Dispatch compute shader work
             * @param groupCountX Local workgroups to dispatch in the X dimension
             * @param groupCountY Local workgroups to dispatch in the Y dimension
             * @param groupCountZ Local workgroups to dispatch in the Z dimension
             */
            virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

            /**
             * Dispatch compute shader work with non-zero base values for workgroup IDs
             * @param baseGroupX Start value for the X component of WorkgroupId
             * @param baseGroupY Start value for the Y component of WorkgroupId
             * @param baseGroupZ Start value for the Z component of WorkgroupId
             * @param groupCountX Local workgroups to dispatch in the X dimension
             * @param groupCountY Local workgroups to dispatch in the Y dimension
             * @param groupCountZ Local workgroups to dispatch in the Z dimension
             */
            virtual void dispatchBase(uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ,
                                      uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

            /**
             * Dispatch compute shader work with indirect parameters
             * @param buffer Buffer containing dispatch parameters
             * @param offset Byte offset into buffer where parameters begin
             */
            virtual void dispatchIndirect(Buffer* buffer, uint64_t offset) override;

            /**
             * Bind a graphics shader pipeline
             * @param pipeline Pipeline to bind
             */
            virtual void bindGraphicsShaderPipeline(ShaderPipeline* pipeline) override;

            /**
             * Bind a compute shader pipeline
             * @param pipeline Pipeline to bind
             */
            virtual void bindComputeShaderPipeline(ShaderPipeline* pipeline) override;

            /**
             * Bind a bundle of descriptors to a slot in a graphics shader pipeline
             * @param index Descriptor Group Index to bind to
             * @param bundle Descriptors to bind to bound shader
             */
            virtual void bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;
            /**
             * Bind a bundle of descriptors to a slot in a compute shader pipeline
             * @param index Descriptor Group Index to bind to
             * @param bundle Descriptors to bind to bound shader
             */
            virtual void bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle) override;

            virtual void pushGraphicsConstants(uint32_t offset, uint32_t size, void* data)override;
            virtual void pushComputeConstants(uint32_t offset, uint32_t size, void* data)override;

            /**
             * Bind a buffer that contains drawing indexes
             * @param buffer Buffer with the index data
             * @param indexSize Size of index (index stride in buffer)
             * @param offset Offset into the buffer the index data begins
             */
            virtual void bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset) override;

            /**
             * Bind buffers that contain drawing vertices
             * @param firstBindingIndex Index to start binding index buffers to (buffers are ordered per shader)
             * @param buffers Buffers containing vertex attribute data
             * @param bufferOffsets Offsets into vertex buffers vertex where data begins in each vertex buffer
             * @param strides Strides of vertex attributes in each vertex buffer
             * @param bufferCount Count of buffers passed in buffers array
             */
            virtual void bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers, uint64_t* bufferOffsets,
                                           uint64_t* strides, uint32_t bufferCount) override;


            //virtual void beginQuery(QueryPool* queryPool, uint32_t query, bool precise)override;
            //virtual void endQuery(QueryPool* pool, uint32_t query)override;
            //virtual void copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride,QueryPool::ResultFlag flags)override;
            //virtual void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)override;

            ID3D12GraphicsCommandList7* dx12Handle();

        protected:
            void move(IDX12CommandBuffer& from);
            GPUQueue::QueueType _queueType = GPUQueue::QueueType::GRAPHICS;
            ID3D12GraphicsCommandList7* _buffer = nullptr;
            ID3D12CommandAllocator* _pool = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12ICOMMANDBUFFER_H
