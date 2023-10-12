#ifndef SLAG_TEXTURESAMPLER_H
#define SLAG_TEXTURESAMPLER_H

#define SAMPLER_ADDRESS_MODES_DEFINTITIONS(DEFINITION) \
DEFINITION(REPEAT,VK_SAMPLER_ADDRESS_MODE_REPEAT,UNDEFINED) \
DEFINITION(MIRRORED_REPEAT,VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,UNDEFINED) \
DEFINITION(CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,UNDEFINED) \
DEFINITION(CLAMP_TO_BORDER,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,UNDEFINED) \


#define SAMPLER_FILTER_DEFINTITIONS(DEFINITION) \
DEFINITION(NEAREST,VK_FILTER_NEAREST ,UNDEFINED) \
DEFINITION(LINEAR,VK_FILTER_LINEAR,UNDEFINED) \
DEFINITION(CUBIC,VK_FILTER_CUBIC_EXT ,UNDEFINED) \


namespace slag
{
    class TextureSampler
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
#define DEFINITION(slagName, vulkanName, dx12Name) slagName,
            SAMPLER_FILTER_DEFINTITIONS(DEFINITION)
#undef DEFINITION
        };
        virtual ~TextureSampler()=default;
        virtual Filter magFilter()=0;
        virtual Filter minFilter()=0;
        virtual AddressMode addressU()=0;
        virtual AddressMode addressV()=0;
        virtual AddressMode addressW()=0;
    };

    class TextureSamplerBuilder
    {
    public:
        TextureSamplerBuilder& setFilters(TextureSampler::Filter filter);
        TextureSamplerBuilder& setMinFilter(TextureSampler::Filter filter);
        TextureSamplerBuilder& setMagFilter(TextureSampler::Filter filter);
        TextureSamplerBuilder& setAddressModes(TextureSampler::AddressMode mode);
        TextureSamplerBuilder& setAddressU(TextureSampler::AddressMode mode);
        TextureSamplerBuilder& setAddressV(TextureSampler::AddressMode mode);
        TextureSamplerBuilder& setAddressW(TextureSampler::AddressMode mode);
        TextureSampler* create();
    private:
        TextureSampler::Filter _minFilter = TextureSampler::Filter::NEAREST;
        TextureSampler::Filter _magFilter = TextureSampler::Filter::NEAREST;
        TextureSampler::AddressMode _addressU = TextureSampler::AddressMode::REPEAT;
        TextureSampler::AddressMode _addressV = TextureSampler::AddressMode::REPEAT;
        TextureSampler::AddressMode _addressW = TextureSampler::AddressMode::REPEAT;
    };
}
#endif //SLAG_TEXTURESAMPLER_H
