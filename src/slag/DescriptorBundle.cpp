#include "DescriptorBundle.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    DescriptorBundle::DescriptorBundle(DescriptorBundle&& from)
    {
        move(std::move(from));
    }

    DescriptorBundle& DescriptorBundle::operator=(DescriptorBundle&& from)
    {
        move(std::move(from));
        return *this;
    }

    const void* DescriptorBundle::gpuHandle() const
    {
        return _gpuhandle;
    }

    void DescriptorBundle::move(DescriptorBundle&& from)
    {
        std::swap(_gpuhandle, from._gpuhandle);
    }

    void DescriptorBundle::setSampler(uint32_t binding, uint32_t arrayElement, Sampler* sampler, Texture::Layout layout)
    {
        lib::BackEndLib::get()->setSampler(_gpuhandle, binding, arrayElement, sampler, layout);
    }

    void DescriptorBundle::setSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture,Texture::Layout layout)
    {
        lib::BackEndLib::get()->setSampledTexture(_gpuhandle, binding, arrayElement, texture, layout);
    }

    void DescriptorBundle::setSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture,Texture::Layout layout, Sampler* sampler)
    {
        lib::BackEndLib::get()->setSamplerAndTexture(_gpuhandle, binding, arrayElement, texture, layout, sampler);
    }

    void DescriptorBundle::setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture,Texture::Layout layout)
    {
        lib::BackEndLib::get()->setStorageTexture(_gpuhandle, binding, arrayElement, texture, layout);
    }

    void DescriptorBundle::setUniformTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)
    {
        lib::BackEndLib::get()->setUniformTexelBuffer(_gpuhandle, binding, arrayElement, buffer, offset, length);

    }

    void DescriptorBundle::setStorageTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)
    {
        lib::BackEndLib::get()->setStorageTexelBuffer(_gpuhandle, binding, arrayElement, buffer, offset, length);

    }

    void DescriptorBundle::setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)
    {
        lib::BackEndLib::get()->setUniformBuffer(_gpuhandle, binding, arrayElement, buffer, offset, length);

    }

    void DescriptorBundle::setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)
    {
        lib::BackEndLib::get()->setStorageBuffer(_gpuhandle, binding, arrayElement, buffer, offset, length);

    }

    void DescriptorBundle::setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture,Texture::Layout layout)
    {
        lib::BackEndLib::get()->setInputAttachment(_gpuhandle, binding, arrayElement, texture, layout);

    }
} // slag