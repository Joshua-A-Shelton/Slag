#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "GPUQueue.h"
#include "ICommandBuffer.h"

namespace slag
{
    class ResourceDescriptorMemory;
    class SamplerDescriptorMemory;
    ///Series of commands that are to be executed on the GPU
    class CommandBuffer: public ICommandBuffer
    {
    public:
        ~CommandBuffer()override = default;
        virtual void bindDescriptorMemory(ResourceDescriptorMemory* resourceDescriptorMemory,SamplerDescriptorMemory* samplerDescriptorMemory)=0;
        static CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands);
        static ICommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer);
    };
} // slag

#endif //SLAG_COMMANDBUFFER_H
