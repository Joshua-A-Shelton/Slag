#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include "GpuQueue.h"

namespace slag
{
    class GraphicsCard
    {
    public:
        virtual ~GraphicsCard()=default;
        virtual GpuQueue* RenderQueue()=0;
        virtual GpuQueue* GraphicsQueue()=0;
        virtual GpuQueue* TransferQueue()=0;
        virtual GpuQueue* ComputeQueue()=0;
    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
