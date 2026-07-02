#ifndef SLAG_SAMPLERDESCRIPTORHEAP_H
#define SLAG_SAMPLERDESCRIPTORHEAP_H
#include <cstdint>

namespace slag
{
    class Sampler;
    class GraphicsCard;
    class SamplerDescriptorHeap
    {
    public:
        virtual ~SamplerDescriptorHeap() = default;
        virtual GraphicsCard* graphicsCard()=0;
        virtual uint64_t size()=0;
        virtual void setSampler(uint64_t heapOffset, Sampler* sampler)=0;
        virtual void* pointer()=0;
        virtual uint64_t deviceAddress()=0;
    };
}
#endif //SLAG_SAMPLERDESCRIPTORHEAP_H