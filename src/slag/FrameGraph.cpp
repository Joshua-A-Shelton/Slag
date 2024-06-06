#include <cassert>
#include "FrameGraph.h"

namespace slag
{
    void FrameGraph::setGlobal(std::string resourceName, Texture *texture)
    {
        assert(resourceName!="output" && "The name \"output\" is reserved. If you were trying to assign the output texture, use the function \"setOutputTexture\" instead");
        _globalTextures[resourceName] = texture;
    }

    void FrameGraph::setOutputTexture(Texture *texture)
    {
        _globalTextures["output"] = texture;
        _outputTexture = texture;
    }

    void FrameGraph::execute(CommandBuffer *on, void *executionData)
    {
        FrameResourceDictionary resources;
        for(auto& tex: _globalTextures)
        {
            resources.assignGlobalTexture(tex.first,tex.second);
        }
        for(auto& stage: _stages)
        {
            stage.execute(on,resources,executionData);
        }
    }

    FrameGraph::FrameGraph(FrameGraph &&from)
    {
        move(std::move(from));
    }

    FrameGraph &FrameGraph::operator=(FrameGraph &&from)
    {
        move(std::move(from));
        return *this;
    }

    Texture *FrameGraph::outputTexture()
    {
        return _outputTexture;
    }

    void FrameGraph::move(FrameGraph &&from)
    {
        _globalTextures.swap(from._globalTextures);
        _stages.swap(from._stages);
        _outputTexture = from._outputTexture;
        for(auto& stage: _stages)
        {
            stage._partOf = this;
        }
    }

    void FrameGraph::addStage(std::vector<GraphicsPass> &passes, PipelineStage::PipelineStageFlags requireFinish, PipelineStage::PipelineStageFlags signalStarted,  std::vector<framegraph::VirtualImageMemoryBarrier> &imageBarriers, std::unordered_map<std::string,TextureResourceDescription>& createTextures, std::unordered_map<std::string,std::string>& aliases)
    {
        FrameGraphStage stage(this);
        stage._imageMemoryBarriers = imageBarriers;
        stage._requireFinish = requireFinish;
        stage._signalStarted = signalStarted;
        stage._createTextures = createTextures;
        stage._aliases = aliases;
        for(auto& pass: passes)
        {
            if(pass._callback != nullptr)
            {
                stage._callbacks.push_back(pass._callback);
            }
        }
        _stages.push_back(std::move(stage));
    }




} // slag