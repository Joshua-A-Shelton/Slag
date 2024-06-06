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
        Pixels::PixelFormat format = Pixels::R32G32B32A32_SFLOAT;
        Texture::Usage usage = Texture::Usage::COLOR;
        bool renderToCapable = false;

        bool equivelentTo(TextureResourceDescription& to)
        {
            if(sizingMode == to.sizingMode && width == to.width && height == to.height && format == to.format && usage == to.usage && renderToCapable == to.renderToCapable)
            {
                return true;
            }
            return false;
        }
    };
    struct TextureResource
    {
        std::string name;
        TextureResourceDescription description;
    };
    struct TextureResourceUsage
    {
        Texture::Layout layout;
        TextureResource resource;
    };
    struct GlobablTextureUsage
    {
        //The texture's name
        std::string name;
        //what the layout needs to be
        Texture::Layout layout;
        Texture::Usage type = Texture::Usage::COLOR;
        //what the texture will be used for
        PipelineAccess::PipeLineAccessFlags usageFlags = PipelineAccess::PipeLineAccessFlags::NONE;
    };


    struct UniformBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
    };

    struct VertexBufferResourceDescription
    {
        uint64_t defaultSize = 1250000;
        Buffer::Usage usage = Buffer::Usage::GPU;
        bool equivelentTo(VertexBufferResourceDescription& to)
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

        bool equivelentTo(IndexBufferResourceDescription& to)
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
