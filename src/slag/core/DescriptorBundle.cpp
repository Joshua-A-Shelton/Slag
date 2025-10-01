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
    void DescriptorBundle::setSampler(DescriptorIndex* index, uint32_t arrayElement, Sampler* sampler)
    {
        Backend::current()->setDescriptorBundleSampler(*this,index,arrayElement,sampler);
    }

    void DescriptorBundle::setSampledTexture(DescriptorIndex* index, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleSampledTexture(*this,index,arrayElement,texture);
    }

    void DescriptorBundle::setStorageTexture(DescriptorIndex* index, uint32_t arrayElement, Texture* texture)
    {
        Backend::current()->setDescriptorBundleStorageTexture(*this,index,arrayElement,texture);
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

    void DescriptorBundle::setUniformTexelBuffer(DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)
    {
        Backend::current()->setDescriptorBundleUniformTexelBuffer(*this,index,arrayElement,bufferView);
    }

    void DescriptorBundle::setStorageTexelBuffer(DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)
    {
        Backend::current()->setDescriptorBundleStorageTexelBuffer(*this,index,arrayElement,bufferView);
    }

    void DescriptorBundle::setUniformBuffer(DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleUniformBuffer(*this,index,arrayElement,buffer,offset,length);
    }

    void DescriptorBundle::setStorageBuffer(DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
    {
        Backend::current()->setDescriptorBundleStorageBuffer(*this,index,arrayElement,buffer,offset,length);
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
