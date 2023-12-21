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
            Absolute,
            FrameRelative
        };
        SizingMode sizingMode = FrameRelative;
        float width = 1.0f;
        float height = 1.0f;
        Pixels::PixelFormat format = Pixels::R32G32B32A32_SFLOAT;
        Texture::Usage usage = Texture::Usage::COLOR;
        bool renderToCapable = false;
    };


    struct UniformBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
    };

    struct VertexBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
        Buffer::Usage usage = Buffer::Usage::GPU;
    };

    struct IndexBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
        Buffer::Usage usage = Buffer::Usage::GPU;
    };
}

#endif //SLAG_RESOURCEDESCRIPTIONS_H