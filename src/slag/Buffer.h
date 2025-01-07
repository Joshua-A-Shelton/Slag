#ifndef SLAG_BUFFER_H
#define SLAG_BUFFER_H

#include <vector>
#include <cstddef>

namespace slag
{
    class Buffer
    {
    public:
        enum Accessibility
        {
            ///Memory is located on host, lowest bandwidth for GPU operations, but most bandwidth for updating from CPU
            CPU = 0b00000001,
            ///Memory is located on GPU highest bandwidth, but inaccessible from host
            GPU = 0b00000010,
            ///Memory is located on GPU, but is mapped to a CPU location. Middle of the road bandwidth for both CPU and GPU operations
            CPU_AND_GPU = 0b00000011
        };

        enum Usage
        {
            ///every buffer is implicitly a data buffer, use this when no other buffer usage flags apply
            DATA_BUFFER = 0b00000000,
            ///Indicates the buffer contains vertex information
            VERTEX_BUFFER = 0b00000001,
            ///Indicates the buffer contains vertex indexes
            INDEX_BUFFER = 0b00000010,
            ///Indicates the buffer is shader writeable
            STORAGE_BUFFER = 0b00000100,
            ///Indicates the buffer is suitable as a parameter to indirect calls
            INDIRECT_BUFFER = 0b00001000
        };
        enum IndexSize
        {
            UINT16,
            UINT32
        };
        virtual ~Buffer()=default;
        ///Send data to buffer from CPU (goes through intermediate buffer automatically if required)
        virtual void update(size_t offset, void* data, size_t dataLength)=0;
        ///get copy of data in buffer, (goes through intermediate buffer automatically if required)
        virtual std::vector<std::byte> downloadData()=0;
        ///size of the buffer in bytes
        virtual size_t size()=0;
        //Whether or not the buffer is natively accessible from the CPU, GPU, or both
        virtual Accessibility accessibility()=0;

        static Buffer* newBuffer(void* data, size_t dataSize, Accessibility accessibility, Usage usage);
        static Buffer* newBuffer(size_t  bufferSize, Accessibility accessibility, Usage usage);

    };

    inline Buffer::Accessibility operator|(Buffer::Accessibility a, Buffer::Accessibility b)
    {
        return static_cast<Buffer::Accessibility>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline Buffer::Accessibility operator|=(Buffer::Accessibility& a, Buffer::Accessibility b)
    {
        a = a|b;
        return a;
    }

    inline Buffer::Accessibility operator&(Buffer::Accessibility a, Buffer::Accessibility b)
    {
        return static_cast<Buffer::Accessibility>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
    }

    inline Buffer::Accessibility operator&=(Buffer::Accessibility& a, Buffer::Accessibility b)
    {
        a = a&b;
        return a;
    }

    inline Buffer::Accessibility operator~(Buffer::Accessibility a)
    {
        return static_cast<Buffer::Accessibility>(~static_cast<unsigned int>(a));
    }

    inline Buffer::Usage operator|(Buffer::Usage a, Buffer::Usage b)
    {
        return static_cast<Buffer::Usage>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
    }

    inline Buffer::Usage operator|=(Buffer::Usage& a, Buffer::Usage b)
    {
        a = a|b;
        return a;
    }

    inline Buffer::Usage operator&(Buffer::Usage a, Buffer::Usage b)
    {
        return static_cast<Buffer::Usage>(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
    }

    inline Buffer::Usage operator&=(Buffer::Usage& a, Buffer::Usage b)
    {
        a = a&b;
        return a;
    }

    inline Buffer::Usage operator~(Buffer::Usage a)
    {
        return static_cast<Buffer::Usage>(~static_cast<unsigned int>(a));
    }

} // slag

#endif //SLAG_BUFFER_H
