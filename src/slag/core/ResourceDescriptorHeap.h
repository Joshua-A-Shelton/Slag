#ifndef SLAG_RESOURCEDESCRIPTORHEAP_H
#define SLAG_RESOURCEDESCRIPTORHEAP_H
#include <cstdint>
#include <slag/core/Pixels.h>

namespace slag
{
    class GraphicsCard;
    class Texture;
    class Buffer;
    ///Chunk of GPU memory that can be used to store descriptors for Textures and Buffers
    class ResourceDescriptorHeap
    {
    public:
        virtual ~ResourceDescriptorHeap() = default;
        ///The Graphics Card that this heap belongs to
        virtual GraphicsCard* graphicsCard()=0;
        ///Total usable bytes in the heap
        virtual uint64_t size()=0;
        ///Pointer to the start of the heap (data is optimized for writes, not reads, treat the same as a buffer with BufferCPUAccess::WRITE_ONLY)
        virtual void* data()=0;
        ///Device address of the start of the heap in GPU memory
        virtual uint64_t deviceAddress()=0;
    };
}
#endif //SLAG_RESOURCEDESCRIPTORHEAP_H