#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include "GpuQueue.h"

namespace slag
{
    class GraphicsCard
    {
    public:
        virtual ~GraphicsCard()=default;
        virtual GpuQueue* graphicsQueue()=0;
        virtual GpuQueue* transferQueue()=0;
        virtual GpuQueue* computeQueue()=0;

        virtual void defragmentMemory()=0;

    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
