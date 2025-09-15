#ifndef SLAG_BUFFERVIEW_H
#define SLAG_BUFFERVIEW_H

#include <slag/core/Buffer.h>

#include "Pixels.h"

namespace slag
{
    class BufferView
    {
    public:
        virtual ~BufferView() = default;
        virtual Buffer* buffer()=0;
        virtual Pixels::Format format()=0;
        virtual uint64_t offset()=0;
        virtual uint64_t size()=0;

        static BufferView* newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size);
    };
} // slag

#endif //SLAG_BUFFERVIEW_H