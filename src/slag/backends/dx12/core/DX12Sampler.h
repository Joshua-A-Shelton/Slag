#ifndef SLAG_DX12SAMPLER_H
#define SLAG_DX12SAMPLER_H

#include <slag/Slag.h>
#include <d3d12.h>
namespace slag
{
    namespace dx12
    {
        class DX12Sampler: public Sampler
        {
        public:
            DX12Sampler(const SamplerParameters& parameters);
            DX12Sampler(const DX12Sampler&)=delete;
            DX12Sampler& operator=(const DX12Sampler&)=delete;
            DX12Sampler(DX12Sampler&& from);
            DX12Sampler& operator=(DX12Sampler&& from);
            virtual ~DX12Sampler()override;
            ///Method to downscale texel data
            virtual Filter minFilter()override;
            ///Method to upscale texel data
            virtual Filter magFilter()override;
            ///Method to scale between mip maps
            virtual Filter mipMapFilter()override;
            ///How to return texel data outside texture's width
            virtual AddressMode addressU()override;
            ///How to return texel data outside texture's height
            virtual AddressMode addressV()override;
            ///How to return texel data outside texture's depth
            virtual AddressMode addressW()override;
            ///Bias towards mip level selection (negative is towards higher level, positive is towards lower level)
            virtual float mipLODBias()override;
            ///If anisotrophic filtering is enabled (reduces blurriness at oblique angles)
            virtual bool anisotrophyEnabled()override;
            ///Maximum levels of anisotrphic filtering possible if enabled
            virtual uint8_t maxAnisotrophy()override;
            ///Comparison applied to texel data before filtering in Depth Comparison
            virtual Operations::ComparisonFunction comparisonFunction()override;
            ///Border color to use in the case CLAMP_TO_BORDER is used
            virtual Color borderColor()override;
            ///Minimum number of mips to use in acquiring texel data
            virtual float minLOD()override;
            ///Maximum number of mips to use in acquiring texel data
            virtual float maxLOD()override;
        private:
            void move(DX12Sampler& from);
            Filter _magFilter;
            Filter _minFilter;
            Filter _mipMapFilter;
            AddressMode _u;
            AddressMode _v;
            AddressMode _w;
            float _mipLODBias=0;
            bool _anisotrophyEnabled=false;
            uint8_t _maxAnisotrophy=1;
            Operations::ComparisonFunction _comparisonFunction;
            Color _borderColor;
            float _minLOD=0;
            float _maxLOD = 1000;

            D3D12_CPU_DESCRIPTOR_HANDLE _descriptorHandle{0};
        };
    } // dx12
} // slag

#endif //SLAG_DX12SAMPLER_H
