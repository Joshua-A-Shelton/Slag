#ifndef SLAG_DX12SAMPLER_H
#define SLAG_DX12SAMPLER_H
#include <slag/Slag.h>

#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        class DX12Sampler: public Sampler
        {
        public:
            DX12Sampler(DX12GraphicsCard* graphicsCard,
                SamplerFilter min,
                SamplerFilter mag,
                SamplerFilter mip,
                SamplerAddressMode u,
                SamplerAddressMode v,
                SamplerAddressMode w,
                float mipLODBias,
                bool anisotrophyEnabled,
                uint8_t  maxAnisotrophy,
                ComparisonFunction comparisonFunction,
                Color borderColor,
                float minLOD,
                float maxLOD);
            DX12Sampler(const DX12Sampler&)=delete;
            DX12Sampler& operator=(const DX12Sampler&)=delete;
            DX12Sampler(DX12Sampler&& from) noexcept;
            DX12Sampler& operator=(DX12Sampler&& from) noexcept;
            ~DX12Sampler()override;
            SamplerFilter minFilter()override;
            SamplerFilter magFilter()override;
            SamplerFilter mipMapFilter()override;
            SamplerAddressMode addressU()override;
            SamplerAddressMode addressV()override;
            SamplerAddressMode addressW()override;
            float mipLODBias()override;
            bool anisotrophyEnabled()override;
            uint8_t maxAnisotrophy()override;
            ComparisonFunction comparisonFunction()override;
            Color borderColor()override;
            float minLOD()override;
            float maxLOD()override;
            const D3D12_SAMPLER_DESC& dx12Desc()const;
            GraphicsCard* graphicsCard()override;

        private:
            void move(DX12Sampler& from);
            D3D12_SAMPLER_DESC _dx12Desc;
            SamplerFilter _min;
            SamplerFilter _mag;
            SamplerFilter _mip;
            SamplerAddressMode _u;
            SamplerAddressMode _v;
            SamplerAddressMode _w;
            float _mipLODBias=0;
            bool _anisotrophyEnabled=false;
            uint8_t  _maxAnisotrophy=1;
            ComparisonFunction _comparisonFunction;
            Color _borderColor;
            float _minLOD=0;
            float _maxLOD=1000;

            DX12GraphicsCard* _graphicsCard=nullptr;
        };
    } // dx
} // slag

#endif //SLAG_DX12SAMPLER_H