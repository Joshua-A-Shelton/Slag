#ifndef SLAG_ICOMMANDBUFFER_H
#define SLAG_ICOMMANDBUFFER_H
#include "GPUQueue.h"

namespace slag
{
    ///Provides the set of universal command buffer commands
    class ICommandBuffer
    {
    public:
        virtual ~ICommandBuffer()=default;
        ///What kinds of commands can this queue execute
        virtual GPUQueue::QueueType commandType()=0;
        ///Begin recording commands
        virtual void begin()=0;
        ///Stop recording commands
        virtual void end()=0;
    };
} // slag

#endif //SLAG_ICOMMANDBUFFER_H
