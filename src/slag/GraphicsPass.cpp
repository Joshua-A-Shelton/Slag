#include <stdexcept>
#include "GraphicsPass.h"

namespace slag
{
    GraphicsPass::GraphicsPass(std::string name, PipelineStage::PipelineStageFlags usage)
    {
        _name = name;
        _stageUsage = usage;
    }

    GraphicsPass::GraphicsPass(const GraphicsPass &from)
    {
        copy(from);
    }
    GraphicsPass &GraphicsPass::operator=(const GraphicsPass& from)
    {
        copy(from);
        return *this;
    }

    GraphicsPass::GraphicsPass(GraphicsPass &&from)
    {
        move(std::move(from));
    }

    GraphicsPass &GraphicsPass::operator=(GraphicsPass &&from)
    {
        move(std::move(from));
        return *this;
    }



    void GraphicsPass::move(slag::GraphicsPass &&from)
    {
        _stageUsage = from._stageUsage;
        _localTextures.swap(from._localTextures);
        _globalUsages.swap(from._globalUsages);
        _callback = from._callback;
        _name.swap(from._name);
    }

    void GraphicsPass::copy(const GraphicsPass &from)
    {
        _stageUsage = from._stageUsage;
        _localTextures = from._localTextures;
        _globalUsages = from._globalUsages;
        _callback = from._callback;
        _name = from._name;
    }

    void GraphicsPass::setCallback(void (*callback)(CommandBuffer *, FrameResourceDictionary&, void *))
    {
        _callback = callback;
    }

    GraphicsPass &GraphicsPass::addLocalTexture(const std::string& name, TextureResourceDescription description, Texture::Layout layout)
    {
        if(name.find(":"))
        {
            throw std::runtime_error("\":\" is a reserved character");
        }
        _localTextures.push_back
        (
                TextureResourceUsage
                {
                    .layout = layout,
                    .resource =
                            {
                                .name = _name+":"+name,
                                .description = description
                            }
                }
        );

        return *this;
    }

    GraphicsPass &GraphicsPass::addGlobalTexture(const std::string& name,Texture::Usage textureType, Texture::Layout layout, PipelineAccess::PipeLineAccessFlags textureUsage)
    {
        if(name.find(":") != std::string::npos)
        {
            throw std::runtime_error("\":\" is a reserved character");
        }
        GlobablTextureUsage texUsage
        {
            .name = name,
            .layout = layout,
            .type = textureType,
            .usageFlags = textureUsage
        };
        _globalUsages[name] = texUsage;
        return *this;
    }


} // slag