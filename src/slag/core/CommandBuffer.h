#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "GPUQueue.h"
namespace slag
{
    ///Series of commands that are to be executed on the GPU
    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer()=default;


        static CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands);
    };
} // slag

#endif //SLAG_COMMANDBUFFER_H
