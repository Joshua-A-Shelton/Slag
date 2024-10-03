#include "Sampler.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    SamplerBuilder& SamplerBuilder::setMinFilter(Sampler::Filter filter)
    {
        _minFilter = filter;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMagFilter(Sampler::Filter filter)
    {
        _magFilter = filter;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMipMapFilter(Sampler::Filter filter)
    {
        _mipMapFilter = filter;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setAddressModeU(Sampler::AddressMode addressMode)
    {
        _u = addressMode;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setAddressModeV(Sampler::AddressMode addressMode)
    {
        _v = addressMode;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setAddressModeW(Sampler::AddressMode addressMode)
    {
        _w = addressMode;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMipLODBias(float bias)
    {
        _mipLODBias = bias;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setAnisotrophyEnabled(bool enabled)
    {
        _anisotrophyEnabled = enabled;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMaxAnisotrophy(uint8_t max)
    {
        _maxAnisotrophy = std::clamp((int)max,1,16);
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setComparisonFunction(Sampler::ComparisonFunction comparisonFunction)
    {
        _comparisonFunction = comparisonFunction;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setBorderColor(Color color)
    {
        _borderColor = color;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMinLOD(float lod)
    {
        _minLOD = lod;
        return *this;
    }

    SamplerBuilder& SamplerBuilder::setMaxLOD(float lod)
    {
        _maxLOD = lod;
        return *this;
    }

    Sampler* SamplerBuilder::newSampler()
    {
        return lib::BackEndLib::get()->newSampler(_minFilter,_magFilter,_mipMapFilter,_u,_v,_w,_mipLODBias,_anisotrophyEnabled,_maxAnisotrophy,_comparisonFunction,_borderColor,_minLOD,_maxLOD);
    }
} // slag