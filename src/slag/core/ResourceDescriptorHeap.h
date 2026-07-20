#ifndef SLAG_RESOURCEDESCRIPTORHEAP_H
#define SLAG_RESOURCEDESCRIPTORHEAP_H
#include <cstdint>
#include <slag/core/Pixels.h>

namespace slag
{
    class GraphicsCard;
    class Texture;
    class Buffer;
    class ResourceDescriptorHeap
    {
    public:
        virtual ~ResourceDescriptorHeap() = default;
        virtual GraphicsCard* graphicsCard()=0;
        virtual uint64_t size()=0;

        virtual void* data()=0;
        virtual uint64_t deviceAddress()=0;
    };
}
#endif //SLAG_RESOURCEDESCRIPTORHEAP_H