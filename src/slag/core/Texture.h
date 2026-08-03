#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H
#include <cstdint>
#include "Dimensions.h"
#include "Pixels.h"

namespace slag
{
    class GraphicsCard;
    ///What the layout of a texture is
    enum class TextureType: uint8_t
    {
        ///Texture has a width
        ONE_DIMENSIONAL,
        ///Texture has a width and height
        TWO_DIMENSIONAL,
        ///Texture has a width, height, and depth
        THREE_DIMENSIONAL,
        ///Texture has six TWO_DIMENSIONAL images, arranged into a cube, accessed by raycast from the center of that cube
        CUBE_MAP
    };
    ///What the texture is being used for, multiple flags can be chained together
    enum class TextureUsageFlags: uint8_t
    {
        NONE = 0,
        ///Texture data is capable of being accessed by a sampler in shaders
        SAMPLED                 = 0b00000001,
        ///Texture supports arbitrary read/write operations in shaders
        READ_WRITE        = 0b00000010,
        ///Texture can be set as a color target of graphics shaders
        COLOR_TARGET            = 0b00000100,
        ///Texture can be set as a depth/stencil target of graphics shaders
        DEPTH_STENCIL_TARGET    = 0b00001000,
    };

    ///How many samples are stored at each texel
    enum class SampleCount: uint8_t
    {
        ONE = 1,
        TWO = 2,
        FOUR = 4,
        EIGHT = 8,
        SIXTEEN = 16,
    };
    ///Structured data for Color or Depth information on the GPU
    class Texture
    {
    protected:
        Texture()=default;
    public:
        virtual ~Texture()=default;
        ///Width in texels
        [[nodiscard]] virtual uint32_t width()const=0;
        ///Height in texels (1 for 1D textures)
        [[nodiscard]] virtual uint32_t height()const=0;
        ///Depth in texels (1 for non-THREE_DIMENSIONAL textures)
        [[nodiscard]] virtual uint32_t depth()const=0;
        ///Array count (1 for non TWO_DIMENSIONAL_ARRAY textures)
        [[nodiscard]] virtual uint32_t layers()const=0;
        ///Number of downsized LOD levels
        [[nodiscard]] virtual uint32_t mipLevels()const=0;
        ///Structure of texels in this texture
        [[nodiscard]] virtual PixelFormat format()const=0;
        ///Number of samples at a given texel
        [[nodiscard]] virtual SampleCount sampleCount()const=0;
        ///The kind of texture this is
        [[nodiscard]] virtual TextureType type()const=0;
        ///What kind of functionality this texture has
        [[nodiscard]] virtual TextureUsageFlags usage()const=0;
        ///Which graphics card this texture is allocated on
        [[nodiscard]] virtual GraphicsCard* graphicsCard()=0;
        ///Pointer to user supplied data (The texture does not own this data, just keeps a reference to it)
        [[nodiscard]] virtual void* userData()=0;
        /**
         * Provide additional data associated to this texture (The texture will not own this data, and it must be managed separately)
         * @param userData Pointer to data to associate with this texture
         */
        virtual void setUserData(void* userData)=0;

        /**
        * Width in texels of a mip level
        * @param mipLevel
        * @return
        */
        [[nodiscard]] uint32_t mipWidth(uint32_t mipLevel)const;
        /**
         * Height in texels of a mip level
         * @return
         */
        [[nodiscard]] uint32_t mipHeight(uint32_t mipLevel)const;
        /**
         * Depth in texels of a mip level
         * @return
         */
        [[nodiscard]] uint32_t mipDepth(uint32_t mipLevel)const;
        /**
         * Get the required size of a buffer needed to contain the data for this texture
         * @param aspect The aspect of the texture to get the size of the buffer for
         * @return
         */
        [[nodiscard]] uint64_t bufferSize(PixelAspect aspect)const;
    };

    inline TextureUsageFlags operator|(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<TextureUsageFlags>(l | r);
    }

    inline TextureUsageFlags operator&(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<TextureUsageFlags>(l & r);
    }

    inline TextureUsageFlags operator^(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<TextureUsageFlags>(l ^ r);
    }

    inline TextureUsageFlags operator~(TextureUsageFlags lhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        return static_cast<TextureUsageFlags>(~l);
    }

    inline TextureUsageFlags operator!(TextureUsageFlags lhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        return static_cast<TextureUsageFlags>(!l);
    }

    inline TextureUsageFlags operator|=(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline TextureUsageFlags operator&=(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    inline TextureUsageFlags operator^=(TextureUsageFlags lhs, TextureUsageFlags rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }
} // slag

#endif //SLAG_TEXTURE_H
