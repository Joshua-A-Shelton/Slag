#include "DX12Sampler.h"
#include "DX12Lib.h"
namespace slag
{
    namespace dx
    {
        DX12Sampler::DX12Sampler(Sampler::Filter minFilter,
                                 Sampler::Filter magFilter,
                                 Sampler::Filter mipMapFilter,
                                 Sampler::AddressMode u,
                                 Sampler::AddressMode v,
                                 Sampler::AddressMode w,
                                 float mipLODBias,
                                 bool anisotrophyEnabled,
                                 uint8_t maxAnisotrophy,
                                 Operations::ComparisonFunction comparisonFunc,
                                 Color borderColor,
                                 float minLOD,
                                 float maxLOD,
                                 bool destroyImmediate): resources::Resource(destroyImmediate)
        {
            _minFilter = minFilter;
            _magFilter = magFilter;
            _mipMapFilter = mipMapFilter;
            _u = u;
            _v = v;
            _w = w;
            _mipLODBias = mipLODBias;
            _anisotrophyEnabled = anisotrophyEnabled;
            _maxAnisotrophy = maxAnisotrophy;
            _comparisonFunction = comparisonFunc;
            _borderColor = borderColor;
            _minLOD = minLOD;
            _maxLOD = maxLOD;

            D3D12_SAMPLER_DESC samplerDesc{};
            samplerDesc.Filter = DX12Lib::filter(minFilter,magFilter,mipMapFilter,anisotrophyEnabled);
            samplerDesc.AddressU = DX12Lib::addressMode(_u);
            samplerDesc.AddressV = DX12Lib::addressMode(_v);
            samplerDesc.AddressW = DX12Lib::addressMode(_w);
            samplerDesc.MipLODBias = _mipLODBias;
            samplerDesc.MaxAnisotropy = _maxAnisotrophy;
            samplerDesc.ComparisonFunc = DX12Lib::comparisonFunction(_comparisonFunction);
            samplerDesc.BorderColor[0] = _borderColor.red;
            samplerDesc.BorderColor[1] = _borderColor.green;
            samplerDesc.BorderColor[2] = _borderColor.blue;
            samplerDesc.BorderColor[3] = _borderColor.alpha;
            samplerDesc.MinLOD = _minLOD;
            samplerDesc.MaxLOD = _maxLOD;

            _descriptorHandle = DX12Lib::card()->getSamplerHandle();

            DX12Lib::card()->device()->CreateSampler(&samplerDesc,_descriptorHandle);
            auto handle = _descriptorHandle;
            _disposeFunction = [=]
            {
                DX12Lib::card()->freeSamplerHandle(handle);
            };
        }

        DX12Sampler::~DX12Sampler()
        {
            if(_descriptorHandle.ptr)
            {
                smartDestroy();
            }
        }

        void DX12Sampler::move(DX12Sampler&& from)
        {
            resources::Resource::move(from);

            _magFilter = from._magFilter;
            _minFilter = from._minFilter;
            _mipMapFilter = from._mipMapFilter;
            _u = from._u;
            _v = from._v;
            _w = from._w;
            _mipLODBias = from._mipLODBias;
            _anisotrophyEnabled = from._anisotrophyEnabled;
            _maxAnisotrophy = from._maxAnisotrophy;
            _comparisonFunction = from._comparisonFunction;
            _borderColor = from._borderColor;
            _minLOD=from._minLOD;
            _maxLOD=from._minLOD;

            std::swap(_descriptorHandle,from._descriptorHandle);
        }

        DX12Sampler::DX12Sampler(DX12Sampler&& from): resources::Resource(from._destroyImmediately)
        {
            move(std::move(from));
        }

        DX12Sampler& DX12Sampler::operator=(DX12Sampler&& from)
        {
            move(std::move(from));
            return *this;
        }

        Sampler::Filter DX12Sampler::magFilter()
        {
            return _magFilter;
        }

        Sampler::Filter DX12Sampler::minFilter()
        {
            return _minFilter;
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
    } // dx
} // slag