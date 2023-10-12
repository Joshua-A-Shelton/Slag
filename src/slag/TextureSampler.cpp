#include "TextureSampler.h"
#include "SlagLib.h"
#ifdef SLAG_VULKAN_BACKEND
#include "BackEnd/Vulkan/VulkanTextureSampler.h"
#endif
namespace slag
{

    TextureSamplerBuilder &TextureSamplerBuilder::setFilters(TextureSampler::Filter filter)
    {
        _minFilter = filter;
        _magFilter = filter;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setMinFilter(TextureSampler::Filter filter)
    {
        _minFilter = filter;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setMagFilter(TextureSampler::Filter filter)
    {
        _magFilter = filter;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setAddressModes(TextureSampler::AddressMode mode)
    {
        _addressU = mode;
        _addressV = mode;
        _addressW = mode;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setAddressU(TextureSampler::AddressMode mode)
    {
        _addressU = mode;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setAddressV(TextureSampler::AddressMode mode)
    {
        _addressV = mode;
        return *this;
    }

    TextureSamplerBuilder &TextureSamplerBuilder::setAddressW(TextureSampler::AddressMode mode)
    {
        _addressW = mode;
        return *this;
    }

    TextureSampler *TextureSamplerBuilder::create()
    {
        switch (SlagLib::usingBackEnd())
        {
            case BackEnd::VULKAN:
#ifdef SLAG_VULKAN_BACKEND
                return new vulkan::VulkanTextureSampler(_minFilter,_magFilter,_addressU,_addressV,_addressW, false);
#endif
                break;
            case BackEnd::DX12:
                break;
        }
        return nullptr;
    }
}