#ifndef SLAG_DX12SAMPLER_H
#define SLAG_DX12SAMPLER_H

#include <directx/d3d12.h>
#include "../../Sampler.h"
#include "../../Resources/Resource.h"

namespace slag
{
    namespace dx
    {

        class DX12Sampler: public Sampler, resources::Resource
        {
        public:
            DX12Sampler(
                    Sampler::Filter minFilter,
                    Sampler::Filter magFilter,
                    Sampler::Filter mipMapFilter,
                    Sampler::AddressMode u,
                    Sampler::AddressMode v,
                    Sampler::AddressMode w,
                    float mipLODBias,
                    bool anisotrophyEnabled,
                    uint8_t maxAnisotrophy,
                    Sampler::ComparisonFunction comparisonFunc,
                    Color borderColor,
                    float minLOD,
                    float maxLOD,
                    bool destroyImmediate);
            ~DX12Sampler();
            DX12Sampler(const DX12Sampler&)=delete;
            DX12Sampler& operator=(const DX12Sampler&)=delete;
            DX12Sampler(DX12Sampler&& from);
            DX12Sampler& operator=(DX12Sampler&& from);
            Filter magFilter()override;
            Filter minFilter()override;
            Filter mipMapFilter()override;
            AddressMode addressU()override;
            AddressMode addressV()override;
            AddressMode addressW()override;
            float mipLODBias()override;
            bool anisotrophyEnabled()override;
            uint8_t maxAnisotrophy()override;
            ComparisonFunction comparisonFunction()override;
            Color borderColor()override;
            float minLOD()override;
            float maxLOD()override;
        private:
            void move(DX12Sampler&& from);
            Filter _magFilter;
            Filter _minFilter;
            Filter _mipMapFilter;
            AddressMode _u;
            AddressMode _v;
            AddressMode _w;
            float _mipLODBias=0;
            bool _anisotrophyEnabled=false;
            uint8_t _maxAnisotrophy=1;
            ComparisonFunction _comparisonFunction;
            Color _borderColor;
            float _minLOD=0;
            float _maxLOD = 1000;

            D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle;
        };

    } // dx
} // slag

#endif //SLAG_DX12SAMPLER_H
