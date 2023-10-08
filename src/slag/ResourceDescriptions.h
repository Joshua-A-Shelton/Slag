#ifndef SLAG_RESOURCEDESCRIPTIONS_H
#define SLAG_RESOURCEDESCRIPTIONS_H
#include "PixelFormat.h"
#include "CommandBuffer.h"
#include "Texture.h"
namespace slag
{
    struct TextureResourceDescription
    {
        enum SizingMode
        {
            ABSOLUTE,
            FRAME_RELATIVE
        };
        SizingMode sizingMode = FRAME_RELATIVE;
        float width = 1.0f;
        float height = 1.0f;
        Pixels::PixelFormat format = Pixels::R32G32B32A32_SFLOAT;
        Texture::Usage usage = Texture::Usage::COLOR;
    };


    struct UniformBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
    };
}

#endif //SLAG_RESOURCEDESCRIPTIONS_H