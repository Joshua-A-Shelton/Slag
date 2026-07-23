#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include <cstdint>

#include "SubmissionQueue.h"
#include "Barriers.h"
#include "Dimensions.h"

namespace slag
{
    class SamplerDescriptorHeap;
    class ResourceDescriptorHeap;
    class Attachment;
    class DescriptorHeap;
    class ShaderPipeline;
    class Buffer;
    class Texture;
    class TextureBufferMapping;

    ///Structure that contains the data for indirect draw calls
    struct IndirectDrawCommand
    {
        ///Number of vertices per instance
        uint32_t vertexCount = 0;
        ///Number of instances
        uint32_t instanceCount = 1;
        ///Offset into bound vertex buffer to start drawing from
        uint32_t firstVertex = 0;
        ///First instance ID (used in shaders)
        uint32_t firstInstance = 0;
    };

    ///Structure that contains the data for indirect indexed draw calls
    struct IndirectDrawIndexedCommand
    {
        ///indexCount Number of indexes per instance
        uint32_t indexCount = 0;
        ///Number of instances
        uint32_t instanceCount = 1;
        ///Offset into bound buffer to start drawing from
        uint32_t firstIndex = 0;
        ///Offset into bound vertex buffer to start drawing from
        int32_t vertexOffset = 0;
        ///First instance ID (used in shaders)
        uint32_t firstInstance = 0;
    };

    ///Structure that contains the data for indirect dispatch calls
    struct IndirectDispatchCommand
    {
        ///Local workgroups to dispatch in the X dimension
        uint32_t groupCountX = 1;
        ///Local workgroups to dispatch in the Y dimension
        uint32_t groupCountY = 1;
        ///Local workgroups to dispatch in the Z dimension
        uint32_t groupCountZ = 1;
    };

    class GraphicsCard;
    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///Which graphics card this command buffer belongs to
        [[nodiscard]] virtual GraphicsCard* graphicsCard()=0;
        ///Which kinds of commands this command buffer can execute
        [[nodiscard]] virtual QueueType type()const=0;
        ///Begin recording commands
        virtual void begin()=0;
        ///End recording commands
        virtual void end()=0;
        /**
         * [TRANSFER] Insert global memory barriers
         * @param barriers Array of global barriers
         * @param barrierCount Number of barriers in barriers array
         */
        virtual void insertBarriers(GlobalBarrier* barriers, uint32_t barrierCount)=0;
        /**
         * [TRANSFER] Insert buffer memory barriers
         * @param barriers Array of buffer barriers
         * @param barrierCount Number of barriers in barriers array
         */
        virtual void insertBarriers(BufferBarrier* barriers, uint32_t barrierCount)=0;
        /**
         * [TRANSFER] Insert texture memory barriers
         * @param barriers Array of texture barriers
         * @param barrierCount Number of barriers in barriers array
         */
        virtual void insertBarriers(TextureBarrier* barriers, uint32_t barrierCount)=0;
        /**
         * [TRANSFER] Insert memory barriers
         * @param globalBarriers Array of global barriers
         * @param globalBarrierCount Number of barriers in globalBarriers array
         * @param bufferBarriers Array of buffer barriers
         * @param bufferBarrierCount Number of barriers in bufferBarriers array
         * @param textureBarriers Array of textureBarriers
         * @param textureBarrierCount Number of barriers in textureBarriers array
         */
        virtual void insertBarriers(
            GlobalBarrier* globalBarriers,
            uint32_t globalBarrierCount,
            BufferBarrier* bufferBarriers,
            uint32_t bufferBarrierCount,
            TextureBarrier* textureBarriers,
            uint32_t textureBarrierCount
            )=0;
        /**
         * [COMPUTE] Binds sections of memory shaders can pull descriptors from
         * @param resourceHeap Heap that holds the resource descriptors
         * @param samplerHeap Heap that holds the sampler descriptors
         */
        virtual void bindDescriptorHeaps(ResourceDescriptorHeap* resourceHeap, SamplerDescriptorHeap* samplerHeap)=0;

        /**
         * [GRAPHICS] Set graphics shader parameter data
         * @param shaderDataOffset Offset into shader data to begin writing. Must be aligned to 4 bytes
         * @param data data to copy into graphics shader data
         * @param dataSize length of data to copy into shader data
         */
        virtual void setGraphicsShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)=0;

        /**
         * [COMPUTE] Set compute shader parameter data
         * @param shaderDataOffset Offset into shader data to begin writing. Must be aligned to 4 bytes
         * @param data data to copy into graphics shader data
         * @param dataSize length of data to copy into shader data
         */
        virtual void setComputeShaderParameters(uint32_t shaderDataOffset, void* data, uint32_t dataSize)=0;

        /**
         * [TRANSFER] Copy data from one buffer to another
         * @param source Buffer to copy from
         * @param sourceOffset Byte offset to start the copy from
         * @param destination Buffer to copy to
         * @param destinationOffset Byte offset to copy the data to
         * @param length Number of bytes to copy
         */
        virtual void copyBufferToBuffer(Buffer* source, uint64_t sourceOffset, Buffer* destination, uint64_t destinationOffset, uint64_t length)=0;
        /**
         * [TRANSFER] Copy data from a texture to a buffer
         * @param source Texture to copy from
         * @param destination Buffer to copy to
         * @param copyData Array of structures containing the parameters of the copy operation
         * @param mappingCount Number of items in copyData array
         */
        virtual void copyTextureToBuffer(Texture* source, Buffer* destination, TextureBufferMapping* copyData, uint32_t mappingCount)=0;
        /**
         * [TRANSFER] Copy data from a buffer to a texture
         * @param source Buffer to copy data from
         * @param destination Texture to copy data to
         * @param copyData Array of structures containing the parameters of the copy operation
         * @param mappingCount Number of items in copyData array
         */
        virtual void copyBufferToTexture(Buffer* source, Texture* destination, TextureBufferMapping* copyData, uint32_t mappingCount)=0;
        /**
         * [GRAPHICS & COMPUTE] Set a shader pipeline as current
         * @param pipeline The pipeline to set as active
         */
        virtual void bindShaderPipeline(ShaderPipeline* pipeline)=0;
        /**
         * [GRAPHICS] Start a renderpass with the given render targets
         * @param colorAttachments Color attachments that shaders will render to in this pa
         * @param colorAttachmentCount Number of Color Attachments
         * @param depthAttachment Depth attachment that shaders will use as depth target in render pass (or nullptr if no depth attachment is needed)
         * @param bounds Area that is affected in render pass
         */
        virtual void beginRendering(Attachment* colorAttachments, uint32_t colorAttachmentCount,Attachment* depthAttachment, const Rectangle& bounds)=0;
        /// [GRAPHICS] End renderpass
        virtual void endRendering()=0;
        /**
        * [GRAPHICS] Sets the drawing area relative to the window, final image will be drawn scaled to the drawing area
        * @param x Pixels away from the left of window to draw
        * @param y Pixels away from the top of the window to draw
        * @param width width in pixels of drawing area
        * @param height height in pixels of drawing area
        * @param minDepth minimum depth of drawn pixels
        * @param maxDepth maximum depth of drawn pixels
        */
        virtual void setViewPort(float x, float y, float width, float height, float minDepth, float maxDepth)=0;
        /**
         * [GRAPHICS] Sets the drawing area relative to the window, final image will be clipped to the drawing area
         * @param rect
         */
        virtual void setScissors(const Rectangle& rect)=0;
        /**
         * [GRAPHICS] Bind a buffer that contains drawing indices
         * @param buffer Buffer containing index data
         * @param indexType If the data in the index buffer is uint16 or uint32
         * @param offset Byte offset into buffer index data begins
         */
        virtual void bindIndexBuffer(Buffer* buffer, IndexBufferType indexType, uint64_t offset)=0;
        /**
         * [GRAPHICS] Bind buffers that contain vertex data
         * @param firstBinding First binding index of the buffers to update
         * @param buffers Buffers to assign as vertex data
         * @param bufferOffsets Offsets into each buffer to start pulling vertex data
         * @param strides Strides of vertex attributes in each vertex buffer
         * @param bufferCount Number of buffers to bind
         */
        virtual void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, uint64_t* bufferOffsets, uint64_t* strides, uint32_t bufferCount)=0;
        /**
         * [GRAPHICS] Draw geometry with currently bound vertex buffers and bound shader
         * @param vertexCount Number of vertices per instance
         * @param instanceCount Number of instances
         * @param firstVertex Offset into bound vertex buffer to start drawing from
         * @param firstInstance First instance ID (used in shaders)
         */
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)=0;
        /**
         * [GRAPHICS] Draw geometry with currently bound vertex buffers, index buffer, and bound shader
         * @param indexCount Number of indexes per instance
         * @param instanceCount Number of instances
         * @param firstIndex Offset into bound buffer to start drawing from
         * @param vertexOffset Offset into bound vertex buffer to start drawing from
         * @param firstInstance First instance ID (used in shaders)
         */
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)=0;
        /**
         * [GRAPHICS] Draw geometry with indirect parameters
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param drawCount Number of draws to exectute, can be zero
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)=0;
        /**
         * [GRAPHICS] Draw geometry with indirect parameters
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param drawCount Number of draws to exectute, can be zero
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndexedIndirect(Buffer* buffer, uint64_t offset, uint32_t drawCount, uint32_t stride)=0;
        /**
         * [GRAPHICS] Draw geometry with indirect parameters and draw count
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param countBuffer Buffer containing the draw count
         * @param countBufferOffset Byte offset into countBuffer where the draw count begins
         * @param maxDrawCount Maximum number of draws that will be executed
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;
        /**
         * [GRAPHICS] Draw geometry with indirect parameters, indexed vertices and draw count
         * @param buffer Buffer that contains drawing parameters
         * @param offset Byte offset into buffer where parameters begin
         * @param countBuffer Buffer containing the draw count
         * @param countBufferOffset Byte offset into countBuffer where the draw count begins
         * @param maxDrawCount Maximum number of draws that will be executed
         * @param stride Byte stride between successive sets of draw parameters
         */
        virtual void drawIndexedIndirectCount(Buffer* buffer, uint64_t offset, Buffer* countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)=0;
        /**
         * [COMPUTE] Dispatch compute shader work
         * @param groupCountX Local workgroups to dispatch in the X dimension
         * @param groupCountY Local workgroups to dispatch in the Y dimension
         * @param groupCountZ Local workgroups to dispatch in the Z dimension
         */
        virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;
        /**
         * [COMPUTE] Dispatch compute shader work with indirect parameters
         * @param buffer Buffer containing dispatch parameters
         * @param offset Byte offset into buffer where parameters begin
         */
        virtual void dispatchIndirect(Buffer* buffer, uint64_t offset)=0;

    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
