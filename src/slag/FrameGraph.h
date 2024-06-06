#ifndef SLAG_FRAMEGRAPHEXECUTEABLE_H
#define SLAG_FRAMEGRAPHEXECUTEABLE_H
#include <string>
#include <unordered_map>
#include <vector>
#include "Texture.h"
#include "FrameGraphStage.h"
#include "GraphicsPass.h"

namespace slag
{

    class FrameGraph
    {
    public:
        FrameGraph(FrameGraph&& from);
        FrameGraph& operator=(FrameGraph&& from);
        FrameGraph(const FrameGraph&)=delete;
        FrameGraph& operator=(const FrameGraph&)=delete;
        void setGlobal(std::string resourceName, Texture* texture);
        void execute(CommandBuffer* on, void* executionData = nullptr);
        void setOutputTexture(Texture* texture);
        Texture* outputTexture();
        friend class FrameGraphBuilder;
    private:
        FrameGraph()=default;
        std::unordered_map<std::string, Texture*> _globalTextures;
        std::vector<FrameGraphStage> _stages;
        Texture* _outputTexture = nullptr;
        void move(FrameGraph&& from);
        void addStage
        (
            std::vector<GraphicsPass> &passes,
            PipelineStage::PipelineStageFlags requireFinish,
            PipelineStage::PipelineStageFlags signalStarted,
            std::vector<framegraph::VirtualImageMemoryBarrier> &imageBarriers,
            std::unordered_map<std::string,TextureResourceDescription>& createTextures,
            std::unordered_map<std::string,std::string>& aliases
        );
    };

} // slag

#endif //CRUCIBLEEDITOR_FRAMEGRAPHEXECUTEABLE_H
