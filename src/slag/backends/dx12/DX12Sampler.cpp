#include "DX12Sampler.h"

#include "DX12Backend.h"

namespace slag
{
    namespace dx12
    {
        DX12Sampler::DX12Sampler(DX12GraphicsCard* graphicsCard, SamplerFilter min, SamplerFilter mag,
            SamplerFilter mip, SamplerAddressMode u, SamplerAddressMode v, SamplerAddressMode w, float mipLODBias,
            bool anisotrophyEnabled, uint8_t maxAnisotrophy, ComparisonFunction comparisonFunction, Color borderColor,
            float minLOD, float maxLOD)
        {
            _graphicsCard = graphicsCard;

            _min = min;
            _mag = mag;
            _mip = mip;
            _u = u;
            _v = v;
            _w = w;
            _mipLODBias = mipLODBias;
            _anisotrophyEnabled = anisotrophyEnabled;
            _maxAnisotrophy = maxAnisotrophy;
            _comparisonFunction = comparisonFunction;
            _borderColor = borderColor;
            _minLOD = minLOD;
            _maxLOD = maxLOD;

            _dx12Desc.Filter = DX12Backend::nativeFilter(_min,_mag,_mip,_anisotrophyEnabled);
            _dx12Desc.AddressU = DX12Backend::nativeAddressMode(_u);
            _dx12Desc.AddressV = DX12Backend::nativeAddressMode(_v);
            _dx12Desc.AddressW = DX12Backend::nativeAddressMode(_w);
            _dx12Desc.MipLODBias = _mipLODBias;
            _dx12Desc.MaxAnisotropy = _maxAnisotrophy;
            _dx12Desc.ComparisonFunc = DX12Backend::nativeCompareFunc(_comparisonFunction);
            _dx12Desc.BorderColor[0] = _borderColor.red;
            _dx12Desc.BorderColor[1] = _borderColor.green;
            _dx12Desc.BorderColor[2] = _borderColor.blue;
            _dx12Desc.BorderColor[3] = _borderColor.alpha;
            _dx12Desc.MinLOD = _minLOD;
            _dx12Desc.MaxLOD = _maxLOD;

        }

        DX12Sampler::DX12Sampler(DX12Sampler&& from) noexcept
        {
            move(from);
        }

        DX12Sampler& DX12Sampler::operator=(DX12Sampler&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12Sampler::~DX12Sampler()
        {
        }

        SamplerFilter DX12Sampler::minFilter()
        {
            return _min;
        }

        SamplerFilter DX12Sampler::magFilter()
        {
            return _mag;
        }

        SamplerFilter DX12Sampler::mipMapFilter()
        {
            return _mip;
        }

        SamplerAddressMode DX12Sampler::addressU()
        {
            return _u;
        }

        SamplerAddressMode DX12Sampler::addressV()
        {
            return _v;
        }

        SamplerAddressMode DX12Sampler::addressW()
        {
            return _w;
        }

        float DX12Sampler::mipLODBias()
        {
            return _mipLODBias;
        }

        bool DX12Sampler::anisotrophyEnabled()
        {
            return _anisotrophyEnabled;
        }

        uint8_t DX12Sampler::maxAnisotrophy()
        {
            return _maxAnisotrophy;
        }

        ComparisonFunction DX12Sampler::comparisonFunction()
        {
            return _comparisonFunction;
        }

        Color DX12Sampler::borderColor()
        {
            return _borderColor;
        }

        float DX12Sampler::minLOD()
        {
            return _minLOD;
        }

        float DX12Sampler::maxLOD()
        {
            return _maxLOD;
        }

        const D3D12_SAMPLER_DESC& DX12Sampler::dx12Desc()const
        {
            return _dx12Desc;
        }

        GraphicsCard* DX12Sampler::graphicsCard()
        {
            return _graphicsCard;
        }

        void DX12Sampler::move(DX12Sampler& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            _min = from._min;
            _mag = from._mag;
            _mip = from._mip;
            _u = from._u;
            _v = from._v;
            _w = from._w;
            _mipLODBias = from._mipLODBias;
            _anisotrophyEnabled = from._anisotrophyEnabled;
            _maxAnisotrophy = from._maxAnisotrophy;
            _comparisonFunction = from._comparisonFunction;
            _borderColor = from._borderColor;
            _minLOD = from._minLOD;
            _maxLOD = from._maxLOD;
        }
    } // dx
} // slag