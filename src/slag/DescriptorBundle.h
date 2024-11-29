#ifndef SLAG_DESCRIPTORBUNDLE_H
#define SLAG_DESCRIPTORBUNDLE_H

#include <cstdint>
#include "Descriptor.h"
#include "Texture.h"

namespace slag
{
    class Buffer;
    class DescriptorBundle
    {
    public:
        DescriptorBundle(const DescriptorBundle&)=delete;
        DescriptorBundle& operator=(const DescriptorBundle&)=delete;
        DescriptorBundle(DescriptorBundle&& from);
        DescriptorBundle& operator=(DescriptorBundle&& from);
        void setSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, Texture::Layout layout);
        void setSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        void setSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout, Sampler* sampler);
        void setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        void setUniformTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        void setStorageTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        void setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        void setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        void setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        //TODO: setAccelerationStructure()
        const void* handle()const;
        friend class DescriptorPool;
    private:
        void move(DescriptorBundle&& from);
        DescriptorBundle()=default;
        void* _handle= nullptr;
    };

} // slag

#endif //SLAG_DESCRIPTORBUNDLE_H
