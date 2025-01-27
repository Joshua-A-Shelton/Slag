#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H

#include "GpuQueue.h"
#include "Clear.h"
#include "GpuMemoryBarriers.h"
#include "Rectangle.h"
#include "Sampler.h"
#include "DescriptorGroup.h"
#include "Attachment.h"
#include "QueryPool.h"
#include "DescriptorBundle.h"

namespace slag
{
    class Texture;
    class Buffer;

    ///Set of common command buffer commands
    class ICommandBuffer
    {
    public:
        ///The type of commands this command buffer can execute
        virtual GpuQueue::QueueType commandType()=0;
        ///Start recording commands
        virtual void begin()=0;
        ///End recording commands
        virtual void end()=0;
        /**
         * Insert GPU barriers that transition texture memory into a different format or require read/write operations to finish before continuing before continuing execution
         * @param imageBarriers Array of Texture related barriers
         * @param imageBarrierCount Number of ImageBarriers
         * @param bufferBarriers Array of Buffer related barriers
         * @param bufferBarrierCount Number of BufferBarriers
         * @param memoryBarriers Array of general memory barriers
         * @param memoryBarrierCount Number of MemoryBarriers
         */
        virtual void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        /**
         * Insert image barrier that transitions texture memory into a different format, and/ or require/read write operations are finished before continuing execution
         * @param barrier
         */
        virtual void insertBarrier(const ImageBarrier& barrier)=0;
        /**
         * Insert buffer barrier that required read/write operations are finished before continuing execution
         * @param barrier
         */
        virtual void insertBarrier(const BufferBarrier& barrier)=0;
        /**
         * Insert generic barrier that requires certain kinds of read/write operations are all finished before continuing execution
         * @param barrier
         */
        virtual void insertBarrier(const GPUMemoryBarrier& barrier)=0;
        /**
         * Clears a color texture
         * @param texture Texture to clear
         * @param color Color to clear to
         * @param currentLayout The layout of the texture at time of execution
         * @param endingLayout the layout to transfer the texture to after clearing
         * @param syncBefore Pipeline stages required to finish before execution of this operation
         * @param syncAfter Pipeline stages required to wait for this operation to finish
         */
        virtual void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        /**
         * Update a texture's mipmap chain
         * @param texture Texture to update
         * @param sourceMipLevel The mipmap level to update the remaining mips from
         * @param sourceLayout The layout of the given mip level at the time of execution
         * @param endingSourceLayout The layout to transfer the source mipmap level to after the operation
         * @param destinationLayout The layout the remaining mipmap images are in at the time of execution
         * @param endingDestinationLayout The layout to transfer the remaining mipmap levels to after the operation
         * @param syncBefore Pipeline stages required to finish before execution of this operation
         * @param syncAfter Pipeline stages required to wait for this operation to finish
         */
        virtual void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        /**
         * Copy the contents of one buffer to another
         * @param source The buffer to copy the data from
         * @param sourceOffset The starting location to copy the data from (in bytes)
         * @param length How much data to copy (in bytes)
         * @param destination The buffer to copy the data to
         * @param destinationOffset The location in the buffer to copy the data to (in bytes)
         */
        virtual void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)=0;
        /**
         * Copy the contents of an image to a buffer
         * @param texture The texture to copy the data from
         * @param layout The layout the texture will be at the time of the operation (TRANSFER_SOURCE/GENERAL)
         * @param baseLayer The index of the texture layer in the texture array (0 for non-arrayed images)
         * @param layerCount The number of texture layers from the array to copy (1 for non-arrayed images)
         * @param mip The mipmap level of the texture to copy
         * @param buffer The buffer to copy the data to
         * @param bufferOffset The offset (in bytes) to copy to
         */
        virtual void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)=0;
        /**
         * Copy the contents of a buffer (texel data) to a texture
         * @param source The buffer containing the texel data to be copied
         * @param sourceOffset The offset (in bytes) to start the copy from
         * @param destination The texture to copy to
         * @param destinationLayout The layout of the texture at the time of execution
         * @param layer The index of the texture layer in the texture array (0 for non-arrayed images) to copy to
         * @param mipLevel The mipmap level of the texture to copy to
         */
        virtual void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)=0;
        /**
         * Draw the contents of one texture into another
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
         * @param filter The filter that handles how drawing resizes is handled
         */
        virtual void blit(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Sampler::Filter filter)=0;

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
        //TODO: Finish documenting command buffer methods
        virtual void beginQuery(QueryPool* queryPool, uint32_t query, bool precise)=0;
        virtual void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment,Rectangle bounds)=0;
        virtual void bindGraphicsDescriptorBundle(ShaderPipeline* shader, uint32_t index, DescriptorBundle& bundle)=0;
        virtual void bindComputeDescriptorBundle(ShaderPipeline* shader, uint32_t index, DescriptorBundle& bundle)=0;
        virtual void bindIndexBuffer(Buffer* buffer,Buffer::IndexSize indexSize, size_t offset)=0;
        virtual void bindGraphicsShader(ShaderPipeline* shader)=0;
        virtual void bindComputeShader(ShaderPipeline* shader)=0;
        virtual void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, size_t* offsets, size_t* sizes, size_t* strides, size_t bindingCount)=0;
        //virtual void clearAttachments(Attachment* attachments, size_t attachmentCount)
        virtual void clearDepthStencilImage(Texture* texture, ClearDepthStencil color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        //virtual void copyImageToImage();
        virtual void copyQueryPoolResults(QueryPool* queryPool, uint32_t firstQuery, uint32_t queryCount, Buffer* destination, size_t offset, size_t stride,QueryPool::ResultFlag flags)=0;
        virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;
        virtual void dispatchBase(uint32_t baseGroupX,uint32_t baseGroupY, uint32_t baseGroupZ,uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;
        virtual void dispatchIndirect(Buffer* buffer, size_t offset)=0;
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)=0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)=0;
        virtual void drawIndexedIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)=0;
        virtual void drawIndexedIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;
        virtual void drawIndirect(Buffer* buffer, size_t offset, uint32_t drawCount, uint32_t stride)=0;
        virtual void drawIndirectCount(Buffer* buffer, size_t offset, Buffer* countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;
        virtual void endQuery(QueryPool* pool, uint32_t query)=0;
        virtual void endRendering()=0;
        //virtual void executeCommands(ICommandBuffer** commands, size_t commandsCount)=0;
        /**
         *
         * @param buffer
         * @param offset
         * @param length the length to fill (must be multiple of 4)
         * @param data
         */
        virtual void fillBuffer(Buffer* buffer, size_t offset, size_t length, uint32_t data)=0;
        virtual void pushConstants(ShaderPipeline* shader, ShaderStages stageFlags, uint32_t offset, uint32_t size, const void* data)=0;
        virtual void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)=0;
        //virtual void resolve(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)=0;

    };
}
#endif //SLAG_ICOMMANDBUFFER_H
