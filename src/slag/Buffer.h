#ifndef SLAG_BUFFER_H
#define SLAG_BUFFER_H

#include <cstdlib>

namespace slag
{
    class Buffer
    {
    public:
        enum Usage
        {
            CPU,
            GPU,
        };
        virtual ~Buffer()=default;
        virtual Usage usage()=0;
        virtual size_t size()=0;
        virtual void update(size_t offset,void* data, size_t dataSize)=0;
    };
}

#endif //SLAG_BUFFER_H