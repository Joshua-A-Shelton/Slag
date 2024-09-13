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
            CPU = 0x00000001,
            GPU = 0x00000010,
            CPU_AND_GPU = 0x00000011
        };

        enum Usage
        {
            //every buffer is implicitly a data buffer, use this when no other buffer usage flags apply
            Data = 0x00000000,
            //Indicates the buffer contains vertex information
            VertexBuffer = 0x00000001,
            //Indicates the buffer contains vertex indexes
            IndexBuffer = 0x00000010,
            //Indicates the buffer is shader writeable
            Storage = 0x00000100,
            //Indicates the buffer is suitable as a parameter to indirect calls
            Indirect = 0x00001000
        };
        virtual ~Buffer()=default;
        virtual void update(size_t offset, void* data, size_t dataLength)=0;
        virtual std::vector<std::byte> downloadData()=0;
        virtual size_t size()=0;
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
