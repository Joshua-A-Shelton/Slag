#ifndef SLAG_FRAMERESOURCEDESCRIPTIONS_H
#define SLAG_FRAMERESOURCEDESCRIPTIONS_H
#include "PixelFormat.h"
#include "Texture.h"
#include "Buffer.h"
#include "GPUMemoryBarriers.h"

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
        int mipLevels = 1;
        Pixels::PixelFormat format = Pixels::R32G32B32A32_SFLOAT;
        Texture::Usage usage = Texture::Usage::COLOR;
        Texture::Features features = Texture::Features::SAMPLED_IMAGE;

        bool equivelentTo(const TextureResourceDescription& to)
        {
            if(sizingMode == to.sizingMode && width == to.width && height == to.height && format == to.format && usage == to.usage && mipLevels == to.mipLevels && features == to.features)
            {
                return true;
            }
            return false;
        }
    };


    struct UniformBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
    };

    struct VertexBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
        Buffer::Usage usage = Buffer::Usage::GPU;
        bool equivelentTo(const VertexBufferResourceDescription& to)
        {
            if(defaultSize == to.defaultSize && usage == to.usage)
            {
                return true;
            }
            return false;
        }
    };

    struct IndexBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
        Buffer::Usage usage = Buffer::Usage::GPU;

        bool equivelentTo(const IndexBufferResourceDescription& to)
        {
            if(defaultSize == to.defaultSize && usage == to.usage)
            {
                return true;
            }
            return false;
        }
    };

}
#endif //CRUCIBLEEDITOR_FRAMERESOURCEDESCRIPTIONS_H
