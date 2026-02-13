#ifndef SLAG_TEXTURE_H
#define SLAG_TEXTURE_H
#include <cstdint>
#include "Pixels.h"
namespace slag
{
    ///What the layout of a texture is
    enum class TextureType
    {
        ///Texture has a width
        ONE_DIMENSIONAL,
        ///Texture has a width and height
        TWO_DIMENSIONAL,
        ///Texture has a width, height, and depth
        THREE_DIMENSIONAL,
        ///Texture has a fixed number (layers) of TWO_DIMENSIONAL images, accessed by index
        TWO_DIMENSIONAL_ARRAY,
        ///Texture has six TWO_DIMENSIONAL images, arranged into a cube, accessed by raycast from the center of that cube
        CUBE_MAP
    };
    ///Structured data for Color or Depth information on the GPU
    class Texture
    {
    protected:
        Texture(TextureType textureType, uint32_t width, uint32_t height, uint32_t depth, uint32_t layers, uint32_t mipMaps, PixelFormat format);
    public:
        virtual ~Texture()=default;
        ///Width in texels
        uint32_t width();
        ///Height in texels (1 for 1D textures)
        uint32_t height();
        ///Depth in texels (1 for non-THREE_DIMENSIONAL textures)
        uint32_t depth();
        ///Array count (1 for non TWO_DIMENSIONAL_ARRAY textures)
        uint32_t layers();
        ///Number of downsized LOD levels
        uint32_t mipLevels();
        /**
         * Width in texels of a mip level
         * @param mipLevel
         * @return
         */
        uint32_t mipWidth(uint32_t mipLevel);
        /**
         * Height in texels of a mip level
         * @return
         */
        uint32_t mipHeight(uint32_t);
        /**
         * Depth in texels of a mip level
         * @return
         */
        uint32_t mipDepth(uint32_t);
        ///Structure of texels in this texture
        PixelFormat format();
    };
} // slag

#endif //SLAG_TEXTURE_H
