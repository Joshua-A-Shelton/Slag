#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include "Attachment.h"
#include "GPUQueue.h"
#include "GPUBarriers.h"
#include "Clear.h"
#include "DescriptorBundle.h"
#include "Dimensions.h"
#include "Sampler.h"
#include "ShaderPipeline.h"
#include "Texture.h"
#include "Buffer.h"

namespace slag
{
    ///Structure that contains the data for indirect draw calls
    struct IndirectDrawCommand
    {
        ///Number of vertices per instance
        uint32_t vertexCount;
        ///Number of instances
        uint32_t instanceCount;
        ///Offset into bound vertex buffer to start drawing from
        uint32_t firstVertex;
        ///First instance ID (used in shaders)
        uint32_t firstInstance;
    };

    ///Structure that contains the data for indirect indexed draw calls
    struct IndirectDrawIndexedCommand
    {
        ///indexCount Number of indexes per instance
        uint32_t indexCount;
        ///Number of instances
        uint32_t instanceCount;
        ///Offset into bound buffer to start drawing from
        uint32_t firstIndex;
        ///Offset into bound vertex buffer to start drawing from
        int32_t vertexOffset;
        ///First instance ID (used in shaders)
        uint32_t firstInstance;
    };
    ///Provides the set of universal command buffer commands
    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///The type of commands this command buffer can execute
        virtual GPUQueue::QueueType commandType()=0;
        ///Start recording commands
        virtual void begin()=0;
        ///End recording commands
        virtual void end()=0;

#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrierDiscreet* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrierDiscreet& barrier)=0;
#else
        /**
         * Insert GPU execution barriers that guarantee certain operations finish before continuing execution of the graphics queue
         * @param textureBarriers Barriers that ensure an operation on a texture finishes before the next operation on it can continue
         * @param textureBarrierCount Number of texture barriers
         * @param bufferBarriers Barriers that ensure an operation on a buffer finishes before the next operation on it can continue
         * @param bufferBarrierCount Number of buffer barriers
         * @param memoryBarriers Barriers that ensure operations in the queue finishes before additional operations can continue
         * @param memoryBarrierCount Number of memory barriers
         */
        virtual void insertBarriers(TextureBarrier* textureBarriers, size_t textureBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GlobalBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        ///Insert GPU execution barrier that ensures an operation on a texture finishes before the next operation on it can continue
        virtual void insertBarrier(const TextureBarrier& barrier)=0;
#endif
        ///Insert GPU execution barrier that ensures an operation on a buffer finishes before the next operation on it can continue
        virtual void insertBarrier(const BufferBarrier& barrier)=0;
        ///Insert GPU execution barrier that ensures an operations in the queue finishes before additional operations can continue
        virtual void insertBarrier(const GlobalBarrier& barrier)=0;

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Clear a color texture
         * @param texture The texture to clear
         * @param color The color to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearColor color)=0;

        /**
         * clear a depth/stencil texture
         * @param texture The texture to clear
         * @param depthStencil The value to clear it to
         */
        virtual void clearTexture(Texture* texture, ClearDepthStencilValue depthStencil)=0;
#else
            /**
         * Clear a color texture
         * @param texture The texture to clear
         * @param textureLayout the layout of the texture being cleared
         * @param color The color to clear it to
         */
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout, ClearColor color)=0;

            /**
             * clear a depth/stencil texture
             * @param texture The texture to clear
             * @param textureLayout the layout of the texture being cleared
             * @param depthStencil The value to clear it to
             */
            virtual void clearTexture(Texture* texture,TextureLayouts::Layout textureLayout ClearDepthStencilValue depthStencil)=0;
#endif

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Propagate an image in a mip chain to a different level in the chain
         * @param texture Texture to update
         * @param layer Texture layer to update
         * @param sourceMip Mip level that acts as a source that is applied to the destination
         * @param destinationMip Mip level to be applied to
         */
        virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip, uint32_t destinationMip)=0;
#else
        /**
        * Propagate an image in a mip chain to a different level in the chain
        * @param texture Texture to update mip for
        * @param layer Texture layer mip update will happen in
        * @param sourceMip Mip level that acts as the source image that gets propagated
        * @param sourceLayout The layout of the source mip at the time of execution (BLIT_SOURCE/GENERAL)
        * @param destinationMip Mip level the source mip is drawn into
        * @param destinationLayout The layout of the destination mip at the time of execution (BLIT_DESTINATION/GENERAL)
        */
        virtual void updateMip(Texture* texture, uint32_t layer, uint32_t sourceMip,TextureLayouts::Layout sourceLayout, uint32_t destinationMip, TextureLayouts::Layout destinationLayout)=0;
#endif

        /**
         * Copy contents of one buffer to another
         * @param source Buffer to copy from
         * @param sourceOffset Offset in the buffer to start copying from
         * @param length Length of data in bytes to copy to the new buffer
         * @param destination Buffer to copy to
         * @param destinationOffset Offset in the buffer to start copying to
         */
        virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, uint64_t length, Buffer* destination, uint64_t destinationOffset)=0;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Copy texel data of a texture to a buffer
         * @param source Texture to copy from
         * @param copyData Parts of the texture to copy
         * @param subresourceCount Number of items in subresources array
         * @param destination Buffer to copy to
         */
        virtual void copyTextureToBuffer(Texture* source, TextureToBufferCopyData* copyData, size_t subresourceCount, Buffer* destination)=0;
#else
         /**
         * Copy texel data of a texture to a buffer
         * @param source Texture to copy from
         * @param textureLayout layout of the source texture
         * @param subresources Parts of the texture to copy
         * @param subresourceCount Number of items in subresources array
         * @param destination Buffer to copy to
         */
         virtual void copyTextureToBuffer(Texture* source, TextureLayouts::Layout textureLayout, TextureSubresource* subresources, size_t subresourceCount, Buffer* destination)=0;
#endif

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
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
        virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea)=0;
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
        virtual void blit(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea,Pixels::AspectFlags aspects)=0;

#else
        /**
        * Draw the contents of one texture into another
        * @param source The texture to copy from
        * @param sourceLayout The layout of the source texture at the time of execution (BLIT_SOURCE/GENERAL)
        * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
        * @param sourceMip The mipmap level of the source texture to copy from
        * @param sourceArea The area of the texture to copy (sized to chosen mip level)
        * @param destination The texture to draw to
        * @param destinationLayout The layout of the destination texture at the time of execution (BLIT_DESTINATION/GENERAL)
        * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
        * @param destinationMip The mipmap level of the source texture to draw to
        * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
        */
        virtual void blit(Texture* source,TextureLayouts::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)=0;
        /**
        * Draw the contents of one texture into another
        * @param source The texture to copy from
        * @param sourceLayout The layout of the source texture at the time of execution (BLIT_SOURCE/GENERAL)
        * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
        * @param sourceMip The mipmap level of the source texture to copy from
        * @param sourceArea The area of the texture to copy (sized to chosen mip level)
        * @param destination The texture to draw to
        * @param destinationLayout The layout of the destination texture at the time of execution (BLIT_DESTINATION/GENERAL)
        * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
        * @param destinationMip The mipmap level of the source texture to draw to
        * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
        * @param aspects The pixel aspects to copy over from the old image to the new
        */
        virtual void blit(Texture* source,TextureLayouts::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Pixels::AspectFlags aspects)=0;
#endif


#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Resolve and draw the contents of a multi-sampled image into another
         * @param source The texture to copy from
         * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
         * @param sourceMip The mipmap level of the source texture to copy from
         * @param sourceArea The area of the texture to copy (sized to chosen mip level)
         * @param destination The texture to draw to
         * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
         * @param destinationMip The mipmap level of the source texture to draw to
         * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
         */
        virtual void resolve(Texture* source, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea);
#else
            /**
             * Resolve and draw the contents of a multi-sampled image into another
             * @param source The texture to copy from
             * @param sourceLayout The layout of the source texture at the time of execution (TRANSFER_SOURCE/GENERAL)
             * @param sourceLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to copy from
             * @param sourceMip The mipmap level of the source texture to copy from
             * @param sourceArea The area of the texture to copy (sized to chosen mip level)
             * @param destination The texture to draw to
             * @param destinationLayout The layout of the destination texture at the time of execution (TRANSFER_DESTINATION/GENERAL)
             * @param destinationLayer The index of the source texture layer in the texture array (0 for non-arrayed images) to draw to
             * @param destinationMip The mipmap level of the source texture to draw to
             * @param destinationArea The area of the texture to draw to (sized to chosen mip level)
             */
            virtual void resolve(Texture* source, TextureLayouts::Layout sourceLayout, uint32_t sourceLayer, uint32_t sourceMip, Rectangle sourceArea, Texture* destination, TextureLayouts::Layout destinationLayout, uint32_t destinationLayer, uint32_t destinationMip, Rectangle destinationArea);
#endif

            /**
             * Fill a buffer with a default value
             * @param buffer Buffer to fill
             * @param offset Byte offset of buffer to start filling from
             * @param length The length to fill (must be multiple of 4)
             * @param data Value to fill the buffer with (repeating)
             */
            virtual void fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)=0;

            /**
            * Sets the drawing area relative to the window, final image will be drawn scaled to the drawing area
            * @param x Pixels away from the left of window to draw
            * @param y Pixels away from the bottom of the window to draw
            * @param width width in pixels of drawing area
            * @param height height in pixels of drawing area
            * @param minDepth minimum depth of drawn pixels
            * @param maxDepth maximum depth of drawn pixels
            */
            virtual void setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)=0;

            /**
             * Sets the drawing area relative to the window, final image will be clipped to the drawing area
             * @param rectangle
             */
            virtual void setScissors(Rectangle rectangle)=0;
            /**
             *
             * @param r Red constant
             * @param g Green constant
             * @param b Blue constant
             * @param a Alpha constant
             */
            virtual void setBlendConstants(float r, float g, float b, float a)=0;
            /**
             * Set the value to compare against in stencil test
             * @param reference
             */
            virtual void setStencilReference(uint32_t reference)=0;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        /**
         * Start a renderpass by providing render targets
         * @param colorAttachments Color attachments that shaders will render to in this pass
         * @param colorAttachmentCount Number of Color Attachments
         * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
         * @param bounds Area that is affected in render pass
         */
        virtual void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment,Rectangle bounds)=0;
#else
            /**
             * Start a renderpass by providing render targets
             * @param colorAttachments Color attachments that shaders will render to in this pass
             * @param colorAttachmentCount Number of Color Attachments
             * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
             * @param bounds Area that is affected in render pass
             */
            virtual void beginRendering(AttachmentDiscreet* colorAttachments, size_t colorAttachmentCount,AttachmentDiscreet* depthAttachment,Rectangle bounds)=0;
#endif
        /**
         * End a renderpass
         */
        virtual void endRendering()=0;

        /**
         * Draw geometry with currently bound vertex buffers and bound shader
         * @param vertexCount Number of vertices per instance
         * @param instanceCount Number of instances
         * @param firstVertex Offset into bound vertex buffer to start drawing from
         * @param firstInstance First instance ID (used in shaders)
         */
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)=0;

        /**
         * Draw geometry with currently bound vertex buffers, index buffer, and bound shader
         * @param indexCount Number of indexes per instance
         * @param instanceCount Number of instances
         * @param firstIndex Offset into bound buffer to start drawing from
         * @param vertexOffset Offset into bound vertex buffer to start drawing from
         * @param firstInstance First instance ID (used in shaders)
         */
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)=0;

        /**
         * Draw geometry with indirect parameters
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param drawCount Number of draws to exectute, can be zero
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)=0;

        /**
         * Draw geometry with indirect parameters, indexed vertices and draw count
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param countBuffer Buffer containing the draw count
         * @param countBufferOffset Byte offset into countBuffer where the draw count begins
         * @param maxDrawCount Maximum number of draws that will be executed
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;

        /**
         * Draw geometry with indirect parameters
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param drawCount Number of draws to exectute, can be zero
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)=0;

        /**
         * Draw geometry with indirect parameters and draw count
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param countBuffer Buffer containing the draw count
         * @param countBufferOffset Byte offset into countBuffer where the draw count begins
         * @param maxDrawCount Maximum number of draws that will be executed
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;

        /**
         * Dispatch compute shader work
         * @param groupCountX Local workgroups to dispatch in the X dimension
         * @param groupCountY Local workgroups to dispatch in the Y dimension
         * @param groupCountZ Local workgroups to dispatch in the Z dimension
         */
        virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;

        /**
         * Dispatch compute shader work with non-zero base values for workgroup IDs
         * @param baseGroupX Start value for the X component of WorkgroupId
         * @param baseGroupY Start value for the Y component of WorkgroupId
         * @param baseGroupZ Start value for the Z component of WorkgroupId
         * @param groupCountX Local workgroups to dispatch in the X dimension
         * @param groupCountY Local workgroups to dispatch in the Y dimension
         * @param groupCountZ Local workgroups to dispatch in the Z dimension
         */
        virtual void dispatchBase(uint32_t baseGroupX,uint32_t baseGroupY, uint32_t baseGroupZ,uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;

        /**
         * Dispatch compute shader work with indirect parameters
         * @param buffer Buffer containing dispatch parameters
         * @param offset Byte offset into buffer where parameters begin
         */
        virtual void dispatchIndirect(Buffer* buffer, size_t offset)=0;

        /**
         * Bind a graphics shader pipeline
         * @param pipeline Pipeline to bind
         */
        virtual void bindGraphicsShaderPipeline(ShaderPipeline* pipeline)=0;

        /**
         * Bind a compute shader pipeline
         * @param pipeline Pipeline to bind
         */
        virtual void bindComputeShaderPipeline(ShaderPipeline* pipeline)=0;

        /**
         * Bind a bundle of descriptors to a slot in a graphics shader pipeline
         * @param index Descriptor Group Index to bind to
         * @param bundle Descriptors to bind to bound shader
         */
        virtual void bindGraphicsDescriptorBundle(uint32_t index, DescriptorBundle& bundle)=0;
        /**
         * Bind a bundle of descriptors to a slot in a compute shader pipeline
         * @param index Descriptor Group Index to bind to
         * @param bundle Descriptors to bind to bound shader
         */
        virtual void bindComputeDescriptorBundle(uint32_t index, DescriptorBundle& bundle)=0;

        /**
         * Bind a buffer that contains drawing indexes
         * @param buffer Buffer with the index data
         * @param indexSize Size of index (index stride in buffer)
         * @param offset Offset into the buffer the index data begins
         */
        virtual void bindIndexBuffer(Buffer* buffer, Buffer::IndexSize indexSize, uint64_t offset)=0;

        /**
         * Bind buffers that contain drawing vertices
         * @param firstBindingIndex Index to start binding index buffers to (buffers are ordered per shader)
         * @param buffers Buffers containing vertex attribute data
         * @param bufferOffsets Offsets into vertex buffers vertex data begins
         * @param bufferCount Count of buffers passed in buffers array
         */
        virtual void bindVertexBuffers(uint32_t firstBindingIndex, Buffer** buffers, uint64_t* bufferOffsets, size_t bufferCount)=0;




        //virtual void beginQuery(QueryPool* queryPool, uint32_t query, bool precise)=0;
        //virtual void endQuery(QueryPool* pool, uint32_t query)=0;
        //virtual void copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride,QueryPool::ResultFlag flags)=0;
        //virtual void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)=0;
    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
