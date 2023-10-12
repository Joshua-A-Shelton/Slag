#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include <cstdlib>
#include "GPUTypes.h"
#include "GPUMemoryBarriers.h"
#include "Buffer.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "Rectangle.h"
#include "Attachment.h"
#include "UniformSetData.h"

namespace slag
{
    class CommandBuffer
    {
    public:
        enum Level
        {
            PRIMARY,
            SECONDARY
        };
        virtual ~CommandBuffer()=default;
        virtual Level level()=0;
        virtual void insertMemoryBarrier(const GPUMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertMemoryBarriers(const GPUMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarrier(const ImageMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertImageBarriers(const ImageMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarrier(const BufferMemoryBarrier& barrier, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBufferBarriers(const BufferMemoryBarrier* barriers, size_t count, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStageFlags source, PipelineStageFlags destination)=0;
        virtual void executeSecondaryCommands(CommandBuffer* subBuffer)=0;
        virtual void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount)=0;
        virtual void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount, Attachment depthAttachment)=0;
        virtual void endTargetFramebuffer()=0;
        virtual void setViewport(Rectangle bounds)=0;
        virtual void setScissors(Rectangle bounds)=0;
        virtual void bindVertexBuffer(Buffer* vertexBuffer)=0;
        virtual void bindIndexBuffer(Buffer* indexBuffer, GraphicsTypes::IndexType indexType)=0;
        virtual void bindShader(Shader* shader)=0;
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)=0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, int32_t vertexOffset, uint32_t firstInstance)=0;
        virtual void bindUniformSetData(Shader* shader, UniformSetData& data)=0;

    };
}
#endif //SLAG_COMMANDBUFFER_H
