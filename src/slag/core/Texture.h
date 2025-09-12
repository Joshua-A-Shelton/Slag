#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H
#include <cstdint>

#include "Pixels.h"

namespace slag
{
    struct TextureBufferMapping;
}

namespace slag
{
    ///Holds texel data for many different kinds of uses, such as color data, depth, cubemaps, etc.
    class Texture
    {
    public:
        enum class Type
        {
            TEXTURE_1D,
            TEXTURE_2D,
            TEXTURE_3D,
            TEXTURE_CUBE,
        };

        enum class UsageFlags: uint8_t
        {
            ///Texture can be sampled from
            SAMPLED_IMAGE=0b00000001,
            ///Texture can have read and writes in the same shader
            STORAGE=0b00000010,
            ///Texture can be written to as a color texture in rasterization shaders
            RENDER_TARGET_ATTACHMENT=0b00000100,
            ///Texture can be written to as a depth texture in rasterization shaders
            DEPTH_STENCIL_ATTACHMENT=0b00001000,
        };

        enum class SampleCount
        {
            ONE = 1,
            TWO = 2,
            FOUR = 4,
            EIGHT = 8,
            SIXTEEN = 16,
            THIRTY_TWO = 32,
            SIXTY_FOUR = 64,
        };

        virtual ~Texture()=default;
        ///What kind of type this texture is
        virtual Type type()=0;
        ///What kind of usage does this texture support
        virtual UsageFlags usageFlags()=0;
        ///Samples used in multisampling
        virtual SampleCount sampleCount()=0;
        ///Width in texels
        virtual uint32_t width()=0;

        /**
         * Width in texels at a given mip level
         * @param mipLevel mip to calculate the pixel width for
         * @return
         */
        uint32_t width(uint32_t mipLevel);
        ///Height in pixels
        virtual uint32_t height()=0;

        /**
         * Height in texels at a given mip level
         * @param mipLevel mip to calculate the pixel height for
         * @return
         */
        uint32_t height(uint32_t mipLevel);
        ///Number of depth slices in 3D textures, 1 in everything else
        virtual uint32_t depth()=0;
        /**
         * Depth in texels at a given mip level
         * @param mipLevel
         * @return
         */
        uint32_t depth(uint32_t mipLevel);
        ///Number of elements in the array (1D or 2D textures), (or 6 in cubemaps, one for each face of the cube), must be 1 in 3d Textures
        virtual uint32_t layers()=0;
        ///Number of mip levels (lower LOD images used in shader sampling)
        virtual uint32_t mipLevels()=0;
        ///The type of texel format backing the image
        virtual Pixels::Format format()=0;
        ///Get the size of the entire texture (assuming tightly packed, may be actually stored differently on hardware) in bytes
        uint64_t byteSize();

        /**
         * Get the size of a mip level in bytes (assuming tightly packed, may be actually stored differently on hardware) in bytes
         * @param mipLevel
         * @return
         */
        uint64_t byteSize(uint32_t mipLevel);

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        static Texture* newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount = SampleCount::ONE);
        static Texture* newTexture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount);
#else
        static Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount);
        static Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, uint32_t providedDataMips, uint32_t providedDataLayers);
#endif

    };

    inline Texture::UsageFlags operator|(const Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        return static_cast<Texture::UsageFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline Texture::UsageFlags operator&(const Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        return static_cast<Texture::UsageFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    inline Texture::UsageFlags operator~(const Texture::UsageFlags& a)
    {
        return static_cast<Texture::UsageFlags>(~static_cast<uint8_t>(a));
    }

    inline Texture::UsageFlags operator^(const Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        return static_cast<Texture::UsageFlags>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
    }

    inline Texture::UsageFlags operator|=(Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        a = a | b;
        return a;
    }

    inline Texture::UsageFlags operator&=(Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        a = a & b;
        return a;
    }

    inline Texture::UsageFlags operator^=(Texture::UsageFlags& a, const Texture::UsageFlags& b)
    {
        a = a ^ b;
        return a;
    }

} // slag

#endif //SLAG_TEXTURE_H
