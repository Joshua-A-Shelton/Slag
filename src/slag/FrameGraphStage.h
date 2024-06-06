#ifndef SLAG_FRAMEGRAPHSTAGE_H
#define SLAG_FRAMEGRAPHSTAGE_H
#include <unordered_map>
#include "Texture.h"
#include "CommandBuffer.h"
#include "GraphicsPass.h"
#include "FrameResourceDescriptions.h"
#include "FrameResourceDictionary.h"
namespace slag
{

    class FrameGraph;
    class FrameGraphStage
    {
    public:
        void execute(CommandBuffer* on, FrameResourceDictionary& resources, void* executionData);
        friend class FrameGraph;
    private:
        FrameGraphStage(FrameGraph* partOf);
        void addResourcesTo(FrameResourceDictionary& resources);
        void setAliasesIn(FrameResourceDictionary& resouces);
        void setResourceLayouts(CommandBuffer* on, FrameResourceDictionary& resources);
        FrameGraph* _partOf = nullptr;
        std::unordered_map<std::string,TextureResourceDescription> _createTextures;
        std::unordered_map<std::string,std::string> _aliases;
        std::vector<void (*)(CommandBuffer* buffer, FrameResourceDictionary& resources, void* executionData)> _callbacks;
        //std::vector<GPUMemoryBarrier> _gpuMemoryBarriers;

        std::vector<framegraph::VirtualImageMemoryBarrier> _imageMemoryBarriers;

        //std::vector<BufferMemoryBarrier> _bufferMemoryBarriers;
        PipelineStage::PipelineStageFlags _requireFinish;
        PipelineStage::PipelineStageFlags _signalStarted;
    };

} // slag

#endif //CRUCIBLEEDITOR_FRAMEGRAPHSTAGE_H
