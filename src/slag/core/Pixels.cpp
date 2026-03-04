#include "Pixels.h"

namespace slag
{
    PixelAspectFlags SLAG_PIXEL_ASPECTS[]
    {
        PixelAspectFlags::NONE_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::DEPTH_FLAG | PixelAspectFlags::STENCIL_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::DEPTH_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::DEPTH_FLAG | PixelAspectFlags::STENCIL_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::DEPTH_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG,
        PixelAspectFlags::COLOR_FLAG
    };

    PixelAspectFlags Pixel::aspectFlags(PixelFormat format)
    {
        return SLAG_PIXEL_ASPECTS[static_cast<uint32_t>(format)];
    }

    struct PixelAspectSizes
    {
        uint32_t color=0;
        uint32_t depth=0;
        uint32_t stencil=0;
    };

    PixelAspectSizes SLAG_PIXEL_ASPECT_SIZES[]
    {
        {0,0,0},
        {16,0,0},
        {16,0,0},
        {16,0,0},
        {12,0,0},
        {12,0,0},
        {12,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {8,0,0},
        {0,4,1},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {0,4,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {0,3,1},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {2,0,0},
        {1,0,0},
        {1,0,0},
        {1,0,0},
        {1,0,0},
        {1,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {8,0,0},
        {8,0,0},
        {16,0,0},
        {16,0,0},
        {16,0,0},
        {16,0,0},
        {8,0,0},
        {8,0,0},
        {16,0,0},
        {16,0,0},
        {2,0,0},
        {2,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {4,0,0},
        {16,0,0},
        {16,0,0},
        {16,0,0},
        {16,0,0},
        {4,0,0},
        {3,0,0},
        {3,0,0},
        {4,0,0},
        {2,0,0}
    };

    uint32_t Pixel::aspectSize(PixelFormat format, PixelAspect aspect)
    {
        auto pixelSize = SLAG_PIXEL_ASPECT_SIZES[static_cast<uint32_t>(format)];
        switch (aspect)
        {
        case PixelAspect::COLOR:
            return pixelSize.color;
        case PixelAspect::DEPTH:
            return pixelSize.depth;
        case PixelAspect::STENCIL:
            return pixelSize.stencil;
        }
        return 0;
    }
} // slag
