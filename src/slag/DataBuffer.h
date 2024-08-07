#ifndef CRUCIBLEEDITOR_DATABUFFER_H
#define CRUCIBLEEDITOR_DATABUFFER_H
#include "Buffer.h"

namespace slag
{

    class DataBuffer: virtual public  Buffer
    {
    public:
        virtual ~DataBuffer()=default;
        static DataBuffer* create(void* data, size_t dataLength, Buffer::Usage usage);
        static DataBuffer* create(size_t size, Buffer::Usage usage);
    };

} // slag

#endif //CRUCIBLEEDITOR_DATABUFFER_H
