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

    class ICommandBuffer
    {
    public:
        virtual GpuQueue::QueueType commandType()=0;
        virtual void begin()=0;
        virtual void end()=0;
        virtual void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount, GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount)=0;
        virtual void clearColorImage(Texture* texture, ClearColor color, Texture::Layout currentLayout, Texture::Layout endingLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        virtual void updateMipChain(Texture* texture, uint32_t sourceMipLevel, Texture::Layout sourceLayout, Texture::Layout endingSourceLayout, Texture::Layout destinationLayout, Texture::Layout endingDestinationLayout, PipelineStages syncBefore, PipelineStages syncAfter)=0;
        virtual void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)=0;
        virtual void copyImageToBuffer(Texture* texture,Texture::Layout layout, uint32_t baseLayer, uint32_t layerCount, uint32_t mip,Buffer* buffer, size_t bufferOffset)=0;
        virtual void copyBufferToImage(Buffer* source, size_t sourceOffset, Texture* destination, Texture::Layout destinationLayout, size_t layer, size_t mipLevel)=0;
        virtual void blit(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip,Rectangle sourceArea, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea,Sampler::Filter filter)=0;

        virtual void setViewPort(float x, float y, float width, float height, float minDepth,float maxDepth)=0;
        virtual void setScissors(Rectangle rectangle)=0;

        virtual void beginQuery(QueryPool* queryPool, uint32_t query, bool precise)=0;
        virtual void beginRendering(Attachment* colorAttachments, size_t colorAttachmentCount,Attachment* depthAttachment,Rectangle bounds)=0;
        virtual void bindGraphicsDescriptorBundle(Shader* shader, uint32_t index, DescriptorBundle& bundle)=0;
        virtual void bindComputeDescriptorBundle(Shader* shader, uint32_t index, DescriptorBundle& bundle)=0;
        virtual void bindIndexBuffer(Buffer* buffer,Buffer::IndexSize indexSize, size_t offset)=0;
        virtual void bindGraphicsShader(Shader* shader)=0;
        virtual void bindComputeShader(Shader* shader)=0;
        virtual void bindVertexBuffers(uint32_t firstBinding, Buffer** buffers, size_t* offsets, size_t bindingCount)=0;
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
        //virtual void pushConstants();
        virtual void resetQueryPool(QueryPool* pool, uint32_t firstQuery, uint32_t queryCount)=0;
        //virtual void resolve(Texture* source,Texture::Layout sourceLayout,uint32_t sourceLayer, uint32_t sourceMip, Texture* destination, Texture::Layout destinationLayout,uint32_t destinationLayer, uint32_t destinationMip,Rectangle destinationArea)=0;

    };
}
#endif //SLAG_ICOMMANDBUFFER_H
