#include "CommandBuffer.h"
#include <slag/backends/Backend.h>
#include <slag/utilities/SLAG_ASSERT.h>
#include <stdexcept>



namespace slag
{
    CommandBuffer* CommandBuffer::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newCommandBuffer(acceptsCommands);
    }

    ICommandBuffer* CommandBuffer::newSubCommandBuffer(CommandBuffer* parentBuffer)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newSubCommandBuffer(parentBuffer);
    }
} // slag
