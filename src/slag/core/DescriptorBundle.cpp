#include "DescriptorBundle.h"
#include <slag/backends/Backend.h>
namespace slag
{
    DescriptorBundle::DescriptorBundle(DescriptorBundle&& from)
    {
        move(from);
    }

    DescriptorBundle& DescriptorBundle::operator=(DescriptorBundle&& from)
    {
        move(from);
        return *this;
    }

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
    void DescriptorBundle::setSampler(uint32_t binding, uint32_t arrayElement, Sampler* sampler)
    {
        Backend::current()->setDescriptorBundleSampler(*this,binding,arrayElement,sampler);
    }

    void DescriptorBundle::setSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleSampledTexture(*this,binding,arrayElement,texture);
    }

    void DescriptorBundle::setTextureAndSampler(uint32_t binding, uint32_t arrayElement, Texture* texture, Sampler* sampler)
    {
        Backend::current()->setDescriptorBundleTextureAndSampler(*this,binding,arrayElement,texture,sampler);
    }

    void DescriptorBundle::setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleStorageTexture(*this,binding,arrayElement,texture);
    }

    void DescriptorBundle::setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleInputAttachment(*this,binding,arrayElement,texture);
    }

#else

    void setSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, TextureLayouts::Layout layout)
    {
        Backend::current()->setDescriptorBundleSampler(*this,binding,arrayElement,sampler,layout);
    }

    void setSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
    {
        Backend::current()->setDescriptorBundleSampledTexture(*this,binding,arrayElement,texture,layout);
    }

    void setSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout, Sampler* sampler)
    {
        Backend::current()->setDescriptorBundleTextureAndSampler(*this,binding,arrayElement,texture,layout,sampler);
    }

    void setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
    {
        Backend::current()->setDescriptorBundleStorageTexture(*this,binding,arrayElement,texture,layout);
    }

    void setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
    {
        Backend::current()->setDescriptorBundleInputAttachment(*this,binding,arrayElement,texture,layout);
    }
#endif

    void DescriptorBundle::setUniformTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleUniformTexelBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    void DescriptorBundle::setStorageTexelBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleStorageTexelBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    void DescriptorBundle::setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleUniformBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    void DescriptorBundle::setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleStorageBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    const void* DescriptorBundle::gpuHandle() const
    {
        return _gpuhandle;
    }

    const void* DescriptorBundle::cpuHandle() const
    {
        return _cpuhandle;
    }

    void DescriptorBundle::move(DescriptorBundle& from)
    {
        std::swap(_gpuhandle, from._gpuhandle);
        std::swap(_cpuhandle, from._cpuhandle);
    }
} // slag
