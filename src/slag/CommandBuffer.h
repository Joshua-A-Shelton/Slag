#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include "ICommandBuffer.h"
#include "DescriptorPool.h"

namespace slag
{

    class CommandBuffer: public ICommandBuffer
    {
    public:
        virtual ~CommandBuffer()=default;

        virtual void waitUntilFinished()=0;
        virtual bool isFinished()=0;
        virtual void bindDescriptorPool(DescriptorPool* pool)=0;

        static CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);

    };

} // slag

#endif //CRUCIBLEEDITOR_COMMANDBUFFER_H
