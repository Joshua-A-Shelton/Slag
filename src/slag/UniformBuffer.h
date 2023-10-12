#ifndef SLAG_UNIFORMBUFFER_H
#define SLAG_UNIFORMBUFFER_H

#include <cstdint>

namespace slag
{
    class UniformSet;
    class BufferWriteData
    {
    private:
        void* _backingBuffer = nullptr;
        uint64_t _location = 0;
        uint64_t _size = 0;

    public:
        BufferWriteData(void* backingBuffer, uint64_t location, uint64_t size)
        {
            _backingBuffer = backingBuffer;
            _location = location;
            _size = size;
        }
        void* backingBuffer()const{return _backingBuffer;}
        uint64_t location()const{return _location;}
        uint64_t size()const{return _size;}

    };
    class UniformBuffer
    {
    public:
        virtual ~UniformBuffer()=default;
        virtual void reset()=0;
        virtual BufferWriteData write(void* data, uint64_t size)=0;
        virtual uint64_t virtualSize()=0;
    };
}
#endif //SLAG_UNIFORMBUFFER_H