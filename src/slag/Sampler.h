#ifndef SLAG_SAMPLER_H
#define SLAG_SAMPLER_H

#define SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION) \
DEFINITION(REPEAT,VK_SAMPLER_ADDRESS_MODE_REPEAT,D3D12_TEXTURE_ADDRESS_MODE_WRAP) \
DEFINITION(MIRRORED_REPEAT,VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,D3D12_TEXTURE_ADDRESS_MODE_MIRROR) \
DEFINITION(CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,D3D12_TEXTURE_ADDRESS_MODE_CLAMP) \
DEFINITION(CLAMP_TO_BORDER,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,D3D12_TEXTURE_ADDRESS_MODE_BORDER) \


#define SAMPLER_FILTER_DEFINTITIONS(DEFINITION) \
DEFINITION(NEAREST,VK_FILTER_NEAREST) \
DEFINITION(LINEAR,VK_FILTER_LINEAR) \

#define SAMPLER_COMPARISON_FUNCTION(DEFINITION) \
DEFINITION(COMPARISION_NONE,VK_COMPARE_OP_NEVER,D3D12_COMPARISON_FUNC_NONE)\
DEFINITION(COMPARISION_NEVER,VK_COMPARE_OP_NEVER,D3D12_COMPARISON_FUNC_NEVER)\
DEFINITION(COMPARISION_LESS,VK_COMPARE_OP_LESS,D3D12_COMPARISON_FUNC_LESS)\
DEFINITION(COMPARISION_LESS_OR_EQUAL,VK_COMPARE_OP_LESS_OR_EQUAL,D3D12_COMPARISON_FUNC_LESS_EQUAL)\
DEFINITION(COMPARISION_GREATER,VK_COMPARE_OP_GREATER,D3D12_COMPARISON_FUNC_GREATER)\
DEFINITION(COMPARISION_GREATER_OR_EQUAL,VK_COMPARE_OP_GREATER_OR_EQUAL,D3D12_COMPARISON_FUNC_GREATER_EQUAL) \
DEFINITION(COMPARISION_EQUAL,VK_COMPARE_OP_EQUAL,D3D12_COMPARISON_FUNC_EQUAL)\
DEFINITION(COMPARISION_NOT_EQUAL,VK_COMPARE_OP_NOT_EQUAL,D3D12_COMPARISON_FUNC_NOT_EQUAL)\
DEFINITION(COMPARISION_ALWAYS,VK_COMPARE_OP_ALWAYS,D3D12_COMPARISON_FUNC_ALWAYS)\

#include "Color.h"
namespace slag
{

    class Sampler
    {
    public:
        enum AddressMode
        {
#define DEFINITION(slagName, vulkanName, dx12Name) slagName,
            SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        enum Filter
        {
#define DEFINITION(slagName, vulkanName) slagName,
            SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        enum ComparisonFunction
        {
#define DEFINITION(slagName, vulkanName, dx12Name) slagName,
            SAMPLER_COMPARISON_FUNCTION(DEFINITION)
#undef DEFINITION
        };

        virtual ~Sampler()=default;
        virtual Filter minFilter()=0;
        virtual Filter magFilter()=0;
        virtual Filter mipMapFilter()=0;
        virtual AddressMode addressU()=0;
        virtual AddressMode addressV()=0;
        virtual AddressMode addressW()=0;
        virtual float mipLODBias()=0;
        virtual bool anisotrophyEnabled()=0;
        virtual uint8_t maxAnisotrophy()=0;
        virtual ComparisonFunction comparisonFunction()=0;
        virtual Color borderColor()=0;
        virtual float minLOD()=0;
        virtual float maxLOD()=0;

    };

    class SamplerBuilder
    {
    public:
        SamplerBuilder& setMinFilter(Sampler::Filter filter);
        SamplerBuilder& setMagFilter(Sampler::Filter filter);
        SamplerBuilder& setMipMapFilter(Sampler::Filter filter);
        SamplerBuilder& setAddressModeU(Sampler::AddressMode addressMode);
        SamplerBuilder& setAddressModeV(Sampler::AddressMode addressMode);
        SamplerBuilder& setAddressModeW(Sampler::AddressMode addressMode);
        SamplerBuilder& setMipLODBias(float bias);
        SamplerBuilder& setAnisotrophyEnabled(bool enabled);
        SamplerBuilder& setMaxAnisotrophy(uint8_t max);
        SamplerBuilder& setComparisonFunction(Sampler::ComparisonFunction comparisonFunction);
        SamplerBuilder& setBorderColor(Color color);
        SamplerBuilder& setMinLOD(float lod);
        SamplerBuilder& setMaxLOD(float lod);
        Sampler* newSampler();

    private:
        Sampler::Filter _minFilter = Sampler::Filter::NEAREST;
        Sampler::Filter _magFilter = Sampler::Filter::NEAREST;
        Sampler::Filter _mipMapFilter = Sampler::Filter::NEAREST;
        Sampler::AddressMode _u = Sampler::AddressMode::REPEAT;
        Sampler::AddressMode _v = Sampler::AddressMode::REPEAT;
        Sampler::AddressMode _w = Sampler::AddressMode::REPEAT;
        float _mipLODBias=0;
        bool _anisotrophyEnabled=false;
        uint8_t _maxAnisotrophy=0;
        Sampler::ComparisonFunction _comparisonFunction;
        Color _borderColor=Color(0.0f,0.0f,0.0f,0.0f);
        float _minLOD = 0;
        float _maxLOD = 1000.0f;

    };

} // slag

#endif //SLAG_SAMPLER_H
