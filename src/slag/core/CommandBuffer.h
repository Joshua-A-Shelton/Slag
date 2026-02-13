#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "ICommandBuffer.h"

namespace slag
{
    class CommandBuffer:public ICommandBuffer
    {
    public:
        ~CommandBuffer()override = default;
    };
} // slag

#endif //SLAG_COMMANDBUFFER_H
