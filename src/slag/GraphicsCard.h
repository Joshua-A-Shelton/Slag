#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include "GpuQueue.h"

namespace slag
{
    ///Representation of hardware that excels at parallel operations
    class GraphicsCard
    {
    public:
        virtual ~GraphicsCard()=default;
        ///Queue that accepts CommandBuffers of type GRAPHICS
        virtual GpuQueue* graphicsQueue()=0;
        ///Queue that accepts CommandBuffers of type TRANSFER (may actually be graphics queue depending on underlying hardware)
        virtual GpuQueue* transferQueue()=0;
        ///Queue that accepts CommandBuffers of type COMPUTE (may actually be graphics queue depending on underlying hardware)
        virtual GpuQueue* computeQueue()=0;
        ///Rearrange memory to be packed together, allowing more room for contiguous memory allocations (do not execute while there are executing or recording command buffers)
        virtual void defragmentMemory()=0;

    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
