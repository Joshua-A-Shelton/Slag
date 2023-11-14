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
            CPU_ONLY,
            GPU_ONLY,
            CPU_TO_GPU,
            GPU_TO_CPU
        };
        enum Type
        {
            VERTEX,
            INDEX,
            STORAGE
        };
        virtual ~Buffer()=default;
        virtual Usage usage()=0;
        virtual size_t size()=0;
        virtual Type type()=0;
        virtual void update(size_t offset,void* data, size_t dataSize)=0;
        static Buffer* create(void* data, size_t bufferSize, Type bufferType, Usage usage);

    };
}

#endif //SLAG_BUFFER_H