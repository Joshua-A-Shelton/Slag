#ifndef SLAG_PIXELS_H
#define SLAG_PIXELS_H
#include <cstdint>

namespace slag
{
    enum class PixelFormat: uint32_t
    {
        UNDEFINED = 0,
        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,
        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,
        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,
        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,
        D32_FLOAT_S8X24_UINT,
        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,
        R11G11B10_FLOAT,
        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,
        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,
        D32_FLOAT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,
        D24_UNORM_S8_UINT,
        R8G8_UNORM,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,
        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,
        R8_UNORM,
        R8_UINT,
        R8_SNORM,
        R8_SINT,
        A8_UNORM,
        R9G9B9E5_SHAREDEXP,
        R8G8_B8G8_UNORM,
        G8R8_G8B8_UNORM,
        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        B8G8R8A8_UNORM,
        B8G8R8X8_UNORM,
        B8G8R8A8_UNORM_SRGB,
        B8G8R8X8_UNORM_SRGB,
        BC6H_UF16,
        BC6H_SF16,
        BC7_UNORM,
        BC7_UNORM_SRGB,
        AYUV,
        NV12,
        OPAQUE_420,
        YUY2,
        B4G4R4A4_UNORM
    };

    enum class PixelAspectFlags: uint8_t
    {
        NONE_FLAG = 0,
        COLOR_FLAG = 1,
        DEPTH_FLAG = 2,
        STENCIL_FLAG = 4,
    };

    enum class PixelAspect: uint8_t
    {
        COLOR = 1,
        DEPTH = 2,
        STENCIL = 4,
    };

    class Pixel
    {
    public:
        /**
         * Get the aspects flags a given format supports
         * @param format Format to get aspect flags for
         * @return
         */
        static PixelAspectFlags aspectFlags(PixelFormat format);
        /**
         * Get the size of an aspect for a pixel on the CPU (or buffer)
         * @param format
         * @param aspect
         * @return
         */
        static uint32_t aspectSize(PixelFormat format, PixelAspect aspect);
    };

    inline PixelAspectFlags operator|(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<PixelAspectFlags>(l | r);
    }

    inline PixelAspectFlags operator&(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<PixelAspectFlags>(l & r);
    }

    inline PixelAspectFlags operator^(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        uint8_t r = static_cast<uint8_t>(rhs);
        return static_cast<PixelAspectFlags>(l ^ r);
    }

    inline PixelAspectFlags operator~(PixelAspectFlags lhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        return static_cast<PixelAspectFlags>(~l);
    }

    inline PixelAspectFlags operator!(PixelAspectFlags lhs)
    {
        uint8_t l = static_cast<uint8_t>(lhs);
        return static_cast<PixelAspectFlags>(!l);
    }

    inline PixelAspectFlags operator|=(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline PixelAspectFlags operator&=(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    inline PixelAspectFlags operator^=(PixelAspectFlags lhs, PixelAspectFlags rhs)
    {
        lhs = lhs ^ rhs;
        return lhs;
    }
} // slag

#endif //SLAG_PIXELS_H
