#include "CommandBuffer.h"
#include "BackEnd/BackEndLib.h"

namespace slag
{

    CommandBuffer* CommandBuffer::newCommandBuffer(GpuQueue::QueueType acceptsCommands)
    {
        return lib::BackEndLib::get()->newCommandBuffer(acceptsCommands);
    }

} // slag