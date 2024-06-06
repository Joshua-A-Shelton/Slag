#include <stdexcept>
#include "FrameGraphStage.h"
#include "FrameGraph.h"

namespace slag
{
    FrameGraphStage::FrameGraphStage(slag::FrameGraph *partOf)
    {
        _partOf = partOf;
    }

    void FrameGraphStage::execute(slag::CommandBuffer *on, FrameResourceDictionary& resources, void *executionData)
    {
        addResourcesTo(resources);
        setAliasesIn(resources);
        setResourceLayouts(on,resources);
        for(auto& callback: _callbacks)
        {
            callback(on,resources,executionData);
        }
    }

    void FrameGraphStage::addResourcesTo(FrameResourceDictionary &resources)
    {
        for(auto& kvpair: _createTextures)
        {
            auto& texture = kvpair.second;
            Texture* tex = nullptr;
            if(texture.sizingMode == TextureResourceDescription::SizingMode::Absolute)
            {
                tex = Texture::create(texture.width,texture.height,texture.format,1,texture.renderToCapable);
            }
            else
            {
                tex = Texture::create(texture.width * _partOf->outputTexture()->width(),texture.height * _partOf->outputTexture()->height(),texture.format,1,texture.renderToCapable);
            }

            resources.assignTransientTexture(kvpair.first,tex);
        }
    }

    void FrameGraphStage::setAliasesIn(FrameResourceDictionary &resouces)
    {
        for(auto& kvpair: _aliases)
        {
            resouces.assignGlobalTexture(kvpair.first,resouces.getTexture(kvpair.second));
        }
    }

    void FrameGraphStage::setResourceLayouts(CommandBuffer *on, FrameResourceDictionary &resources)
    {
        std::vector<GPUMemoryBarrier> gpuMemoryBarriers;
        //TODO gpu memory barriers

        std::vector<ImageMemoryBarrier> textureBarriers;
        for(auto& texture: _imageMemoryBarriers)
        {
            ImageMemoryBarrier texBarrier
            {
                .oldLayout = texture.oldLayout,
                .newLayout = texture.newLayout,
                .requireCachesFinish = texture.requireCachesFinish,
                .usingCaches = texture.usingCaches,
                .texture = resources.getTexture(texture.textureName)
            };
            textureBarriers.push_back(texBarrier);
        }

        std::vector<BufferMemoryBarrier> bufferBarriers;
        //TODO: buffer barriers

        on->insertBarriers(gpuMemoryBarriers.data(),gpuMemoryBarriers.size(),textureBarriers.data(),textureBarriers.size(),bufferBarriers.data(),bufferBarriers.size(),_requireFinish,_signalStarted);
    }


} // slag