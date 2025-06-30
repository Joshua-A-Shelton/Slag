#include "CommandBuffer.h"

#include <stdexcept>

namespace slag
{
    CommandBuffer* CommandBuffer::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
    {
        throw std::runtime_error("CommandBuffer::newCommandBuffer: Not Implemented");
    }
} // slag
