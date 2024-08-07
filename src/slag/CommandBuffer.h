#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include <cstdlib>
#include "GPUTypes.h"
#include "GPUMemoryBarriers.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "ComputeShader.h"
#include "UniformBuffer.h"
#include "Rectangle.h"
#include "Viewport.h"
#include "Attachment.h"
#include "UniformSetData.h"
#include "PushConstantRange.h"

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
        virtual void insertBarriers(const GPUMemoryBarrier* memoryBarriers, size_t memoryBarrierCount, const ImageMemoryBarrier* imageBarriers, size_t imageBarrierCount, const BufferMemoryBarrier* bufferBarriers, size_t bufferBarrierCount, PipelineStage::PipelineStageFlags requireFinish, PipelineStage::PipelineStageFlags signalStarted)=0;
        virtual void executeSecondaryCommands(CommandBuffer* subBuffer)=0;
        virtual void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount)=0;
        virtual void setTargetFramebuffer(Rectangle bounds,Attachment* colorAttachments, size_t colorCount, Attachment depthAttachment)=0;
        virtual void endTargetFramebuffer()=0;
        virtual void setViewport(Viewport bounds)=0;
        virtual void setViewport(Rectangle bounds)=0;
        virtual void setScissors(Rectangle bounds)=0;
        virtual void bindVertexBuffer(VertexBuffer* vertexBuffer)=0;
        virtual void bindIndexBuffer(IndexBuffer* indexBuffer, GraphicsTypes::IndexType indexType)=0;
        virtual void bindShader(Shader* shader)=0;
        virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)=0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, int32_t vertexOffset, uint32_t firstInstance)=0;
        virtual void bindUniformSetData(Shader* shader, UniformSetData& data)=0;
        virtual void pushConstants(Shader* shader,PushConstantRange* pushRange, void* data)=0;
        virtual void bindShader(ComputeShader* shader)=0;
        virtual void bindUniformSetData(ComputeShader* shader, UniformSetData& data)=0;
        virtual void pushConstants(ComputeShader* shader,PushConstantRange* pushRange, void* data)=0;
        virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)=0;
        virtual CommandBuffer* createSubCommandBuffer()=0;
        virtual void addSubCommandBuffer(CommandBuffer* buffer)=0;
        virtual void blitImage(Texture* source, Rectangle sourceArea, Texture::Layout sourceLayout, Texture* destination, Rectangle destinationArea, Texture::Layout destinationLayout,TextureSampler::Filter filter)=0;

    };
}
#endif //SLAG_COMMANDBUFFER_H
