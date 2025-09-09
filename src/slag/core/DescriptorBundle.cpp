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

    void DescriptorBundle::setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleStorageTexture(*this,binding,arrayElement,texture);
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

    void setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
    {
        Backend::current()->setDescriptorBundleStorageTexture(*this,binding,arrayElement,texture,layout);
    }
#endif

    void DescriptorBundle::setUniformTexelBuffer(uint32_t binding, uint32_t arrayElement, BufferView* bufferView)
    {
        Backend::current()->setDescriptorBundleUniformTexelBuffer(*this,binding,arrayElement,bufferView);
    }

    void DescriptorBundle::setStorageTexelBuffer(uint32_t binding, uint32_t arrayElement, BufferView* bufferView)
    {
        Backend::current()->setDescriptorBundleStorageTexelBuffer(*this,binding,arrayElement,bufferView);
    }

    void DescriptorBundle::setUniformBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleUniformBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    void DescriptorBundle::setStorageBuffer(uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleStorageBuffer(*this,binding,arrayElement,buffer,offset,length);
    }

    void* DescriptorBundle::gpuHandle() const
    {
        return _gpuhandle;
    }

    void* DescriptorBundle::cpuHandle() const
    {
        return _cpuhandle;
    }

    void DescriptorBundle::move(DescriptorBundle& from)
    {
        std::swap(_gpuhandle, from._gpuhandle);
        std::swap(_cpuhandle, from._cpuhandle);
    }
} // slag
