#ifndef SLAG_RESOURCEDESCRIPTORHEAP_H
#define SLAG_RESOURCEDESCRIPTORHEAP_H
#include <cstdint>
#include <slag/core/Pixels.h>

namespace slag
{
    class ResourceDescriptorHeap
    {
    public:
        virtual ~ResourceDescriptorHeap() = default;
        virtual GraphicsCard* graphicsCard()=0;
        virtual uint32_t descriptorCount()=0;
        virtual void setUniformTexture(uint32_t index, Texture* texture)=0;
        virtual void setUnorderedAccessTexture(uint32_t index, Texture* texture)=0;
        virtual void setUniformBuffer(uint32_t index, Buffer* buffer, uint32_t offset, uint32_t length)=0;
        virtual void setStorageBuffer(uint32_t index, Buffer* buffer, uint32_t offset, uint32_t length)=0;
        virtual void setUniformTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format, uint32_t offset, uint32_t length)=0;
        virtual void setStorageTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format, uint32_t offset, uint32_t length)=0;
    };
}
#endif //SLAG_RESOURCEDESCRIPTORHEAP_H