#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "GPUQueue.h"
#include "ICommandBuffer.h"
namespace slag
{
    class DescriptorPool;
    ///Series of commands that are to be executed on the GPU
    class CommandBuffer: public ICommandBuffer
    {
    public:
        ~CommandBuffer()override = default;
        virtual void bindDescriptorPool(DescriptorPool* pool)=0;

        static CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands);
        static ICommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer);
    };
} // slag

#endif //SLAG_COMMANDBUFFER_H
