#ifndef SLAG_SAMPLER_H
#define SLAG_SAMPLER_H
#include "Color.h"
#include "Operations.h"

namespace slag
{
    class GraphicsCard;
    enum class SamplerAddressMode
    {
        REPEAT,
        MIRRORED_REPEAT,
        CLAMP_TO_EDGE,
        CLAMP_TO_BORDER
    };
    enum class SamplerFilter
    {
        NEAREST,
        LINEAR
    };
    class Sampler
    {
    public:
        virtual ~Sampler() = default;
        ///Method to downscale texel data
        virtual SamplerFilter minFilter()=0;
        ///Method to upscale texel data
        virtual SamplerFilter magFilter()=0;
        ///Method to scale between mip maps
        virtual SamplerFilter mipMapFilter()=0;
        ///How to return texel data outside texture's width
        virtual SamplerAddressMode addressU()=0;
        ///How to return texel data outside texture's height
        virtual SamplerAddressMode addressV()=0;
        ///How to return texel data outside texture's depth
        virtual SamplerAddressMode addressW()=0;
        ///Bias towards mip level selection (negative is towards higher level, positive is towards lower level)
        virtual float mipLODBias()=0;
        ///If anisotrophic filtering is enabled (reduces blurriness at oblique angles)
        virtual bool anisotrophyEnabled()=0;
        ///Maximum levels of anisotrphic filtering possible if enabled
        virtual uint8_t maxAnisotrophy()=0;
        ///Comparison applied to texel data before filtering in Depth Comparison
        virtual ComparisonFunction comparisonFunction()=0;
        ///Border color to use in the case CLAMP_TO_BORDER is used
        virtual Color borderColor()=0;
        ///Minimum number of mips to use in acquiring texel data
        virtual float minLOD()=0;
        ///Maximum number of mips to use in acquiring texel data
        virtual float maxLOD()=0;
        ///The graphics card this sampler is allocated on
        virtual GraphicsCard* graphicsCard()=0;
    };
}
#endif //SLAG_SAMPLER_H