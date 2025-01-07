#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include "ICommandBuffer.h"
#include "DescriptorPool.h"

namespace slag
{

    ///Represents a list of commands that is sent and executed on the GPU
    class CommandBuffer: public ICommandBuffer
    {
    public:
        virtual ~CommandBuffer()=default;
        ///Wait on CPU until command buffer has finished executing after being sent to GPU queue
        virtual void waitUntilFinished()=0;
        ///Is the command buffer finished executing commands
        virtual bool isFinished()=0;
        ///make descriptor pool data available for use in this command buffer, only a single pool can be bound at a time
        virtual void bindDescriptorPool(DescriptorPool* pool)=0;

        /**
         * Creates a new command buffer
         * @param acceptsCommands which type of commands this command buffer can execute/ which kind of queue will process this buffer
         * @return
         */
        static CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);

    };

} // slag

#endif //CRUCIBLEEDITOR_COMMANDBUFFER_H
