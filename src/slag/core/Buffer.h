#ifndef SLAG_BUFFER_H
#define SLAG_BUFFER_H
#include <cstdint>
#include <cstddef>

namespace slag
{
    class SemaphoreValue;
    ///Section of arbitrary memory
    class Buffer
    {
    public:
        enum class UsageFlags:uint8_t
        {
            ///every buffer is implicitly a data buffer, use this when no other buffer usage flags apply
            DATA_BUFFER = 0b00000000,
            ///Indicates the buffer contains vertex information
            VERTEX_BUFFER = 0b00000001,
            ///Indicates the buffer contains vertex indexes
            INDEX_BUFFER = 0b00000010,
            ///Indicates the buffer contains uniform buffer data
            UNIFORM_BUFFER = 0b00000100,
            ///Indicates the buffer is shader writeable
            STORAGE_BUFFER = 0b00001000,
            ///Indicates the buffer contains texel data
            UNIFORM_TEXEL_BUFFER = 0b00010000,
            ///Indicates the buffer contains texel data and is shader writable
            STORAGE_TEXEL_BUFFER = 0b00100000,
            ///Indicates the buffer is suitable as a parameter to indirect calls
            INDIRECT_BUFFER = 0b01000000
        };
        enum class Accessibility
        {
            GPU=0b00000001,
            CPU_AND_GPU = 0b000000110
        };
        ///Type of numeric inside an index array
        enum class IndexSize
        {
            UINT16,
            UINT32
        };

        virtual ~Buffer()=default;
        ///Who can natively access this buffer
        virtual Accessibility accessibility()=0;
        ///Size in bytes of the buffer
        virtual uint64_t size()=0;

        /**
         * Send data to buffer from CPU (goes through intermediate buffer automatically if required)
         * @param offset position in the buffer to start updating
         * @param data the new data to put into the buffer
         * @param dataLength length of new data being inserted into the buffer
         * @param wait semaphores to wait to be signaled before performing the update
         * @param waitCount number of semaphores in wait array
         * @param signal semaphores to signal when update is finished
         * @param signalCount number of semaphores in signal array
         */
        virtual void update(uint64_t offset, void* data, uint64_t dataLength,SemaphoreValue* wait, size_t waitCount,SemaphoreValue* signal, size_t signalCount)=0;
        ///The location of the buffer in host memory (if cpu accessible)
        virtual void* cpuHandle()=0;
        ///CPU handle as a pointer to an object
        template<class T> T* as()
        {
            return static_cast<T*>(cpuHandle());
        }
        ///Number of elements of a given type assuming the buffer is an array of those objects
        template<class T> uint64_t countAsArray()
        {
            return size() / sizeof(T);
        }

        static Buffer* newBuffer(void* data, size_t dataSize, Accessibility accessibility,UsageFlags usage = UsageFlags::DATA_BUFFER);
        static Buffer* newBuffer(size_t size, Accessibility accessibility,UsageFlags usage= UsageFlags::DATA_BUFFER);
    };
} // slag

#endif //SLAG_BUFFER_H
