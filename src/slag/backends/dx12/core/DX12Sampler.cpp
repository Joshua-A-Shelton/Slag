#include "DX12Sampler.h"

#include "DX12GraphicsCard.h"
#include "slag/backends/dx12/DX12Backend.h"

namespace slag
{
    namespace dx12
    {
        DX12Sampler::DX12Sampler(const SamplerParameters& parameters)
        {
            _minFilter = parameters.minFilter;
            _magFilter = parameters.magFilter;
            _mipMapFilter = parameters.mipMapFilter;
            _u = parameters.u;
            _v = parameters.v;
            _w = parameters.w;
            _mipLODBias = parameters.mipLODBias;
            _anisotrophyEnabled = parameters.anisotrophyEnabled;
            _maxAnisotrophy = parameters.maxAnisotrophy;
            _comparisonFunction = parameters.comparisonFunction;
            _borderColor = parameters.borderColor;
            _minLOD = parameters.minLOD;
            _maxLOD = parameters.maxLOD;

            D3D12_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter = DX12Backend::dx12Filter(_minFilter,_magFilter,_mipMapFilter,_anisotrophyEnabled);
            samplerDesc.AddressU = DX12Backend::dx12AddressMode(_u);
            samplerDesc.AddressV = DX12Backend::dx12AddressMode(_v);
            samplerDesc.AddressW = DX12Backend::dx12AddressMode(_w);
            samplerDesc.MipLODBias = _mipLODBias;
            samplerDesc.MaxAnisotropy = _maxAnisotrophy;
            samplerDesc.ComparisonFunc = DX12Backend::dx12ComparisonFunction(_comparisonFunction);
            samplerDesc.BorderColor[0] = _borderColor.red;
            samplerDesc.BorderColor[1] = _borderColor.green;
            samplerDesc.BorderColor[2] = _borderColor.blue;
            samplerDesc.BorderColor[3] = _borderColor.alpha;
            samplerDesc.MinLOD = _minLOD;
            samplerDesc.MaxLOD = _maxLOD;

            _descriptorHandle = DX12GraphicsCard::selected()->getSamplerHandle();

            DX12GraphicsCard::selected()->device()->CreateSampler(&samplerDesc,_descriptorHandle);
        }

        DX12Sampler::DX12Sampler(DX12Sampler&& from)
        {
            move(from);
        }

        DX12Sampler& DX12Sampler::operator=(DX12Sampler&& from)
        {
            move(from);
            return *this;
        }

        DX12Sampler::~DX12Sampler()
        {
            if (_descriptorHandle.ptr)
            {
                DX12GraphicsCard::selected()->freeSamplerHandle(_descriptorHandle);
            }
        }

        Sampler::Filter DX12Sampler::minFilter()
        {
            return _minFilter;
        }

        Sampler::Filter DX12Sampler::magFilter()
        {
            return _magFilter;
        }

        Sampler::Filter DX12Sampler::mipMapFilter()
        {
            return _mipMapFilter;
        }

        Sampler::AddressMode DX12Sampler::addressU()
        {
            return _u;
        }

        Sampler::AddressMode DX12Sampler::addressV()
        {
            return _v;
        }

        Sampler::AddressMode DX12Sampler::addressW()
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

        Operations::ComparisonFunction DX12Sampler::comparisonFunction()
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

        void DX12Sampler::move(DX12Sampler& from)
        {
            _magFilter = from._magFilter;
            _minFilter = from._minFilter;
            _mipMapFilter = from._mipMapFilter;
            _u = from._u;
            _v = from._v;
            _w = from._w;
            _mipLODBias= from._mipLODBias;
            _anisotrophyEnabled= from._anisotrophyEnabled;
            _maxAnisotrophy= from._maxAnisotrophy;
            _comparisonFunction = from._comparisonFunction;
            _borderColor = from._borderColor;
            _minLOD= from._minLOD;
            _maxLOD = from._maxLOD;

            std::swap(_descriptorHandle,from._descriptorHandle);
        }
    } // dx12
} // slag
