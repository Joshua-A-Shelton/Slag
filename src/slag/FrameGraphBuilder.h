#ifndef SLAG_FRAMEGRAPH_H
#define SLAG_FRAMEGRAPH_H
#include "Texture.h"
#include "CommandBuffer.h"
#include "GraphicsPass.h"
#include "FrameGraphStage.h"
#include "FrameGraph.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace slag
{

    class FrameGraphBuilder
    {
    public:
        void addPass(GraphicsPass graphicsPass);
        void addPass(GraphicsPass&& graphicsPass);
        void setGlobablStartingLayout(const std::string& name, Texture::Layout startingLayout);
        FrameGraph compile();
    private:
        std::vector<GraphicsPass> _graphicsPasses;
        std::unordered_map<std::string,Texture::Layout> _defaultLayouts;
        void buildPassData
        (GraphicsPass& currentPass,
         std::unordered_map<std::string,GlobablTextureUsage>& textureStates,
         std::unordered_map<std::string,GlobablTextureUsage>& newStates,
         std::unordered_map<std::string,GlobablTextureUsage>& changedStates,
         std::vector<GraphicsPass>& passes,
         std::vector<framegraph::VirtualImageMemoryBarrier>& memoryBarriers,
         std::unordered_map<std::string,TextureResourceDescription>& aliasableTextures,
         std::unordered_map<std::string,std::string>& aliases,
         std::unordered_set<std::string>& claimedAliases,
         std::unordered_map<std::string,TextureResourceDescription>& createTextures);


    };

} // slag

#endif //CRUCIBLEEDITOR_FRAMEGRAPH_H
