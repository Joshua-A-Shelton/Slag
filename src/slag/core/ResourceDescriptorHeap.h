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
        virtual uint32_t descriptorCount()=0;
        /**
         * Set a descriptor in the heap for a uniform texture
         * @param heapOffset Byte offset into the heap to set the texture descriptor
         * @param texture The texture to set
         * @param baseMip Lowest accessible mip that can be sampled (useful for streaming in higher resolution texture mips)
         * @param mipCount Number of mip levels of the texture that are accessible
         * @param baseLayer Lowest accessible layer that can be sampled
         * @param layerCount Number of layers of the texture that are accessible
         */
        virtual void setUniformTexture(uint32_t heapOffset, Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)=0;
        virtual void setUnorderedAccessTexture(uint32_t heapOffset, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)=0;
        virtual void setUniformStructuredBuffer(uint32_t heapOffset, Buffer* buffer, uint32_t offset, uint32_t length)=0;
        virtual void setStorageStructuredBuffer(uint32_t heapOffset, Buffer* buffer, uint64_t elementIndex, uint64_t elementCount, uint64_t elementStride)=0;
        virtual void setUniformTexelBuffer(uint32_t heapOffset, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)=0;
        virtual void setStorageTexelBuffer(uint32_t heapOffset, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)=0;
    };
}
#endif //SLAG_RESOURCEDESCRIPTORHEAP_H