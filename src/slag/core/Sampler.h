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

#include <cstdint>

#include "Color.h"
#include "Operations.h"

namespace slag
{
    struct SamplerParameters;
    class Sampler
    {
    public:
        ///How to return pixel data beyond the actual bounds of a texture
        enum class AddressMode
        {
#define DEFINITION(slagName, vulkanName, dx12Name) slagName,
            SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        ///Method to upscale/downscale texel data
        enum class Filter
        {
#define DEFINITION(slagName, vulkanName) slagName,
            SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };

        virtual ~Sampler()=default;
        ///Method to downscale texel data
        virtual Filter minFilter()=0;
        ///Method to upscale texel data
        virtual Filter magFilter()=0;
        ///Method to scale between mip maps
        virtual Filter mipMapFilter()=0;
        ///How to return texel data outside texture's width
        virtual AddressMode addressU()=0;
        ///How to return texel data outside texture's height
        virtual AddressMode addressV()=0;
        ///How to return texel data outside texture's depth
        virtual AddressMode addressW()=0;
        ///Bias towards mip level selection (negative is towards higher level, positive is towards lower level)
        virtual float mipLODBias()=0;
        ///If anisotrophic filtering is enabled (reduces blurriness at oblique angles)
        virtual bool anisotrophyEnabled()=0;
        ///Maximum levels of anisotrphic filtering possible if enabled
        virtual uint8_t maxAnisotrophy()=0;
        ///Comparison applied to texel data before filtering in Depth Comparison
        virtual Operations::ComparisonFunction comparisonFunction()=0;
        ///Border color to use in the case CLAMP_TO_BORDER is used
        virtual Color borderColor()=0;
        ///Minimum number of mips to use in acquiring texel data
        virtual float minLOD()=0;
        ///Maximum number of mips to use in acquiring texel data
        virtual float maxLOD()=0;

        static Sampler* newSampler(const SamplerParameters& parameters);
    };
    struct SamplerParameters
    {
        Sampler::Filter minFilter = Sampler::Filter::NEAREST;
        Sampler::Filter magFilter = Sampler::Filter::NEAREST;
        Sampler::Filter mipMapFilter = Sampler::Filter::NEAREST;
        Sampler::AddressMode u = Sampler::AddressMode::REPEAT;
        Sampler::AddressMode v = Sampler::AddressMode::REPEAT;
        Sampler::AddressMode w = Sampler::AddressMode::REPEAT;
        float mipLODBias=0;
        bool anisotrophyEnabled=false;
        uint8_t maxAnisotrophy=0;
        Operations::ComparisonFunction comparisonFunction = Operations::ComparisonFunction::COMPARISION_NEVER;
        Color borderColor=Color(0.0f,0.0f,0.0f,0.0f);
        float minLOD = 0;
        float maxLOD = 1000.0f;
    };
} // slag

#endif //SLAG_SAMPLER_H
