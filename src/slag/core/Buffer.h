#ifndef SLAG_BUFFER_H
#define SLAG_BUFFER_H
#include <cstdint>
#include "Pixels.h"
namespace slag
{
    class GraphicsCard;
    class Sampler;

    ///Indicates what memory caching will be used in shaders
    enum class BufferMemoryType
    {
        ///Shaders can only read data in the buffer (very fast in shader execution, but is read only). Buffers of this type must be a multiple of 256 bytes, and are often limited in max size (16-64 KB usually, depending on hardware)
        UNIFORM,
        ///Shaders can read and write data in the buffer (generally slower in shader execution, but can be arbitrary sizes and may or may not be read only). Intended for general purpose buffers
        GENERAL
    };
    ///Read/Write permission for a buffer from the CPU
    enum class BufferCPUAccess
    {
        ///The CPU have no access to the buffer (generally fastest)
        NONE,
        ///The CPU can write data to the buffer, but not read from it (generally faster, should be same speed as NONE if GraphicsCard::cacheCoherentSharedMemory() is true, but may be limited in size if not)
        WRITE_ONLY,
        ///The CPU can read and write data in the buffer (generally slower, often located in system RAM rather than the graphics card, but may be required for large buffers if WRITE_ONLY is limited)
        READ_WRITE
    };
    ///Chunk of GPU accessible memory
    class Buffer
    {
    protected:
        Buffer()=default;
    public:
        virtual ~Buffer()=default;
        ///Pointer to the beginning of the buffer, throws an error if the CPU doesn't have access to the buffer if SLAG_DEBUG is defined
        [[nodiscard]] virtual void* data()const=0;
        ///Virtual address, useful for bindless shader access
        [[nodiscard]] virtual uint64_t deviceAddress()const=0;
        ///Read/Write permission for shader access to this buffer
        [[nodiscard]] virtual BufferMemoryType memoryType()const=0;
        ///Read/Write permission for cpu access to this buffer
        [[nodiscard]] virtual BufferCPUAccess cpuAccess()const=0;
        ///Number of bytes in buffer
        [[nodiscard]] virtual uint64_t size()const=0;
        ///Which graphics card this buffer is allocated on
        [[nodiscard]] virtual GraphicsCard* graphicsCard()const=0;
        ///Pointer to user supplied data (The Buffer does not own this data, just keeps a reference to it)
        [[nodiscard]] virtual void* userData()=0;
        /**
         * Provide additional data associated to this buffer (The buffer will not own this data, and it must be managed separately)
         * @param userData Pointer to data to associate with this buffer
         */
        virtual void setUserData(void* userData)=0;

        /**
         * CPU accessible pointer as pointer to specific type
         * @tparam T Type to interpret the pointer as
         * @return
         */
        template<class T> T* as()
        {
            return static_cast<T*>(data());
        }

        /**
         * Count of items in buffer when buffer is treated as an array of items of a specific type
         * @tparam T Type of items in buffer
         * @return
         */
        template<class T> [[nodiscard]] uint64_t arrayCount() const
        {
            return size()/sizeof(T);
        }
    };
} // slag

#endif //SLAG_BUFFER_H
