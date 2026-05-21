#ifndef SLAG_SAMPLERDESCRIPTORHEAP_H
#define SLAG_SAMPLERDESCRIPTORHEAP_H
#include <cstdint>

namespace slag
{
    class Sampler;

    class SamplerDescriptorHeap
    {
    public:
        virtual ~SamplerDescriptorHeap() = default;
        virtual GraphicsCard* graphicsCard()=0;
        virtual uint32_t descriptorCount()=0;
        virtual void setSampler(uint32_t index, Sampler* sampler)=0;
    };
}
#endif //SLAG_SAMPLERDESCRIPTORHEAP_H