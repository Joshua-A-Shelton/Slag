#ifndef SLAG_DESCRIPTORHEAP_H
#define SLAG_DESCRIPTORHEAP_H
#include <cstdint>

#include "Texture.h"

namespace slag
{
    class Sampler;
    class GraphicsCard;
    class Texture;
    class Buffer;

    enum class DescriptorHeapType
    {
        RESOURCE,
        SAMPLER
    };

    class DescriptorHeap
    {
    public:
        virtual ~DescriptorHeap() = default;
        virtual void writeSamplerDescriptor(uint32_t location, Sampler* sampler)=0;
        virtual void writeSampledTextureDescriptor(uint32_t location, Texture* texture)=0;
        virtual void writeUnorderedAccessTextureDescriptor(uint32_t location, Texture* texture)=0;
        virtual void writeUniformBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        virtual void writeUnorderedAccessBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        virtual void writeUniformTexelBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length, PixelFormat format)=0;
        virtual void writeUnorderedAccessTexelBufferDescriptor(uint32_t location, Buffer* buffer, uint64_t offset, uint64_t length, PixelFormat format)=0;

        virtual GraphicsCard* graphicsCard()=0;
        virtual uint32_t size() =0;
        virtual uint32_t reserved() =0;
        virtual DescriptorHeapType type()=0;
    };
}
#endif //SLAG_DESCRIPTORHEAP_H