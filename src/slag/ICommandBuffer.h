#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H

#include "GpuQueue.h"
#include "Clear.h"
#include "GpuMemoryBarriers.h"
#include "Rectangle.h"
#include "Sampler.h"

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
        //virtual void blit(Texture* source, Texture::Layout sourceLayout, Rectangle sourceRect, Texture* destination, Texture::Layout destinationLayout, Rectangle destinationRect,Sampler::Filter blitFilter)=0;
        //virtual void blit(Texture* source, Texture::Layout sourceLayout, Rectangle sourceRect,size_t sourceMipLevel, size_t sourceLayer, Texture* destination, Texture::Layout destinationLayout, Rectangle destinationRect, size_t destinationMipLevel, size_t destinationLayer, Sampler::Filter blitFilter)=0;
        virtual void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)=0;
    };
}
#endif //SLAG_ICOMMANDBUFFER_H
