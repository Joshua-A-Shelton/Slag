#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H

#include "GpuQueue.h"
#include "Clear.h"
#include "GpuMemoryBarriers.h"
#include "Texture.h"
#include "Buffer.h"
#include "Rectangle.h"

namespace slag
{
    class ICommandBuffer
    {
    public:
        virtual GpuQueue::QueueType commandType()=0;
        virtual void begin()=0;
        virtual void end()=0;
        virtual void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)=0;
        virtual void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)=0;
        virtual void copyBuffer(Buffer* source, size_t sourceOffset, size_t length, Buffer* destination, size_t destinationOffset)=0;
    };
}
#endif //SLAG_ICOMMANDBUFFER_H
