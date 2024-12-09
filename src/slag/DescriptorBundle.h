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
        /**
         * Assign a sampler to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param sampler The sampler to assign
         * @param layout The layout the texture(s) being sampled will be in during shader execution
         */
        void setSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, Texture::Layout layout);
        /**
         * Assign a texture (that will be sampled) to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param texture The texture to assign
         * @param layout The layout the texture will be in during shader execution
         */
        void setSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        /**
         * Assign a texture and corresponding sampler together to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param texture The texture to assign
         * @param layout The layout the texture will be in during shader execution
         * @param sampler The sampler to assign
         */
        void setSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout, Sampler* sampler);
        /**
         * Assign a texture that can have both read/write operations performed on it (usually compute shaders) to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param texture The texture to assign
         * @param layout The layout the texture will be in during shader execution (UNORDERED or GENERAL)
         */
        void setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        /**
         * Assign a tightly packed 1D array of texels that image sampling operations can be performed on to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param buffer The texel buffer to assign
         * @param offset How far into the buffer to bind as the texel data
         * @param length The length from the offset of the buffer to use as texel data
         */
        void setUniformTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        /**
         * Assign a tightly packed 1D array of texels that both read/write operations can be performed on (usually compute shaders) to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param buffer The texel buffer to assign
         * @param offset How far into the buffer to bind as the texel data
         * @param length The length from the offset of the buffer to use as texel data
         */
        void setStorageTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        /**
         * Assign arbitrary data to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param buffer The buffer to assign
         * @param offset How far into the buffer the data is located
         * @param length The size of the data
         */
        void setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        /**
         * Assign a section of a buffer that can have both read/write operations performed on it (usually compute shaders) to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param buffer The buffer to assign
         * @param offset How far into the buffer the data is located
         * @param length The size of the data
         */
        void setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length);
        /**
         * Assign a texture that can be used for framebuffer local operations to the descriptor bundle
         * @param binding The binding index of the descriptor
         * @param arrayElement The index of the array to assign
         * @param texture The texture to assign
         * @param layout The layout the texture will be in during shader execution
         */
        void setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout);
        //TODO: setAccelerationStructure()
        /**
         * Low level handle to the underlying object, used internally, not terribly useful generally
         * @return
         */
        const void* handle()const;
        friend class DescriptorPool;
    private:
        void move(DescriptorBundle&& from);
        DescriptorBundle()=default;
        void* _handle= nullptr;
    };

} // slag

#endif //SLAG_DESCRIPTORBUNDLE_H
