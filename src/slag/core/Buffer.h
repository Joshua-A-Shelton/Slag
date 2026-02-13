#ifndef SLAG_BUFFER_H
#define SLAG_BUFFER_H
#include <cstdint>

namespace slag
{
    ///What kind of data the buffer will hold
    enum class BufferUsage
    {
        ///Buffer contains generalized data, suitable for holding data for shaders to read or write to
        ARBITRARY,
        ///Buffer contains texel data, and shaders can automatically convert between compatible formats when reading or writing
        TEXEL,
        ///Buffer contains vertex attribute data, and is passed to shaders via binding in CommandBuffer:bindVertexBuffers
        VERTEX,
        ///Buffer contains vertex indices, and is used for retrieving vertices while drawing, and is bound in CommandBuffer:bindIndexBuffer
        INDEX,
        ///Buffer contains data to be used as parameters in indirect drawing commands
        INDIRECT,
    };
    ///Read/Write permission for a buffer from a shader
    enum class BufferShaderAccess
    {
        ///Shaders can only read data in the buffer, but not write to it (generally faster in shader execution)
        READ_ONLY,
        ///Shaders can read and write data in the buffer (generally slower in shader execution)
        READ_WRITE,
    };
    ///Read/Write permission for a buffer from the CPU
    enum class BufferCPUAccess
    {
        ///The CPU had no access to the buffer (generally fastest)
        NONE,
        ///The CPU can write data to the buffer, but not read from it (generally faster, should be same speed as NONE if GraphicsCard::cacheCoherentSharedMemory() is true)
        WRITE_ONLY,
        ///The CPU can read and write data in the buffer (generally slower)
        READ_WRITE
    };
    ///Chunck of GPU accessible memory
    class Buffer
    {
    protected:
        Buffer();
    public:
        virtual ~Buffer()=default;
        ///Pointer to the beginning of the buffer, throws error if the CPU doesn't have access to the buffer
        [[nodiscard]] virtual void* data()const=0;
        ///Virtual address, useful for bindless shader access
        [[nodiscard]] virtual uint64_t deviceAddress()const=0;
        ///The kind of data this buffer holds
        [[nodiscard]] virtual BufferUsage usage()const;
        ///Read/Write permission for shader access to this buffer
        [[nodiscard]] virtual BufferShaderAccess shaderAccess()const;
        ///Read/Write permission for cpu access to this buffer
        [[nodiscard]] virtual BufferCPUAccess cpuAccess()const;
        ///Number of bytes in buffer
        [[nodiscard]] virtual uint64_t size()const;

        /**
         * CPU accessible pointer as pointer to specific type
         * @tparam T Type to interpret the pointer as
         * @return
         */
        template<class T> T* as()
        {
            return static_cast<T*>(data);
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
