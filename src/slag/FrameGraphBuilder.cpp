#include <cassert>
#include "FrameGraphBuilder.h"
#include <unordered_set>
#include <unordered_map>
#include <stdexcept>

namespace slag
{


    void FrameGraphBuilder::addPass(GraphicsPass graphicsPass)
    {
        _graphicsPasses.push_back(graphicsPass);
    }

    void FrameGraphBuilder::addPass(GraphicsPass &&graphicsPass)
    {
        _graphicsPasses.push_back(std::move(graphicsPass));
    }

    void FrameGraphBuilder::setGlobablStartingLayout(const std::string& name, Texture::Layout startingLayout)
    {
        _defaultLayouts[name] = startingLayout;
    }

    FrameGraph FrameGraphBuilder::compile()
    {
        std::unordered_set<std::string> neededTextures;
        std::vector<GraphicsPass> prunedPasses;
        neededTextures.insert("output");
        //loop from the end towards the beginning
        for(auto i=_graphicsPasses.size()-1; i>=0; i--)
        {
            auto& stage = _graphicsPasses[i];
            //check to see if any of the global resources contribute to the final image
            for(auto& resource: stage._globalUsages)
            {
                if(neededTextures.contains(resource.first))
                {
                    prunedPasses.push_back(stage);
                    for(auto& requiredGlobal: stage._globalUsages)
                    {
                        neededTextures.insert(requiredGlobal.first);
                    }
                    break;
                }
            }
            //dealing with the fact we're using unsigned variables to keep track of i
            if(i == 0)
            {
                break;
            }
        }

        std::vector<GraphicsPass> passes;
        std::unordered_map<std::string,GlobablTextureUsage> textureStates;
        std::vector<framegraph::VirtualImageMemoryBarrier> memoryBarriers;

        std::unordered_map<std::string,TextureResourceDescription> aliasableTextures;
        std::unordered_map<std::string,std::string> aliases;
        std::unordered_set<std::string> claimedAliases;
        std::unordered_map<std::string,TextureResourceDescription> createTextures;

        PipelineStage::PipelineStageFlags lastStage = PipelineStage::PipelineStageFlags::TOP;
        PipelineStage::PipelineStageFlags currentStage = prunedPasses[prunedPasses.size()-1]._stageUsage;



        FrameGraph fg;
        //because we went bottom up, our pruned passes or ordered in reverse
        for(auto i=prunedPasses.size()-1; i>=0; i--)
        {
            auto& currentPass = prunedPasses[i];
            std::unordered_map<std::string,GlobablTextureUsage> changedStates;
            std::unordered_map<std::string,GlobablTextureUsage> newStates;
            //get global texture state changes
            for(auto& kvpair: currentPass._globalUsages)
            {
                auto& textureUsage = kvpair.second;
                auto& textureName = kvpair.first;
                if(textureStates.contains(textureName))
                {
                    auto& oldUsage = textureStates[textureName];
                    if(textureUsage.layout != oldUsage.layout || textureUsage.usageFlags != oldUsage.usageFlags)
                    {
                        changedStates[textureName] = textureUsage;
                    }
                }
                else
                {
                    newStates[textureName] = textureUsage;
                }
            }



            if(changedStates.empty() && currentPass._stageUsage == currentStage)
            {
                buildPassData(currentPass,textureStates,changedStates,newStates,passes,memoryBarriers,aliasableTextures,aliases,claimedAliases,createTextures);
                newStates.clear();
                changedStates.clear();
            }
            else
            {
                PipelineStage::PipelineStageFlags last = lastStage;
                PipelineStage::PipelineStageFlags current = currentStage;
                fg.addStage(passes,last,current,memoryBarriers,createTextures,aliases);
                lastStage = currentStage;
                currentStage = currentPass._stageUsage;
                passes.clear();
                memoryBarriers.clear();
                aliasableTextures.insert(createTextures.begin(),createTextures.end());
                aliases.clear();
                claimedAliases.clear();
                createTextures.clear();
                buildPassData(currentPass,textureStates,newStates,changedStates,passes,memoryBarriers,aliasableTextures,aliases,claimedAliases,createTextures);
                newStates.clear();
                changedStates.clear();
            }
            if(i == 0)
            {
                break;
            }
        }
        PipelineStage::PipelineStageFlags last = lastStage;
        PipelineStage::PipelineStageFlags current = currentStage;
        fg.addStage(passes,last,current,memoryBarriers,createTextures,aliases);
        return fg;
    }

    void FrameGraphBuilder::buildPassData(GraphicsPass& currentPass,
                                          std::unordered_map<std::string,GlobablTextureUsage>& textureStates,
                                          std::unordered_map<std::string,GlobablTextureUsage>& newStates,
                                          std::unordered_map<std::string,GlobablTextureUsage>& changedStates,
                                          std::vector<GraphicsPass>& passes,
                                          std::vector<framegraph::VirtualImageMemoryBarrier>& memoryBarriers,
                                          std::unordered_map<std::string,TextureResourceDescription>& aliasableTextures,
                                          std::unordered_map<std::string,std::string>& aliases,
                                          std::unordered_set<std::string>& claimedAliases,
                                          std::unordered_map<std::string,TextureResourceDescription>& createTextures)
    {
        for(auto& kvPair: newStates)
        {
            Texture::Layout startingLayout = Texture::Layout::UNDEFINED;
            if(_defaultLayouts.contains(kvPair.first))
            {
                startingLayout = _defaultLayouts[kvPair.first];
            }

            memoryBarriers.push_back(framegraph::VirtualImageMemoryBarrier
            {
                 //not sure if I can not require caches to finish or not....
                 .oldLayout = startingLayout,
                 .newLayout = kvPair.second.layout,
                 .requireCachesFinish = PipelineAccess::PipeLineAccessFlags::NONE,
                 .usingCaches = kvPair.second.usageFlags,
                 .textureName = kvPair.first
            });
            //add new state to global states
            textureStates[kvPair.first] = kvPair.second;

        }
        for(auto& kvPair: changedStates)
        {
            auto& oldTexState = textureStates[kvPair.first];
            memoryBarriers.push_back(framegraph::VirtualImageMemoryBarrier
            {
                    .oldLayout = oldTexState.layout,
                    .newLayout = kvPair.second.layout,
                    .requireCachesFinish = oldTexState.usageFlags,
                    .usingCaches = kvPair.second.usageFlags,
                    .textureName = kvPair.first
            });
            textureStates[kvPair.first] = kvPair.second;
        }

        for(auto& localTexture: currentPass._localTextures)
        {
            bool foundAlias = false;
            for(auto& alias: aliasableTextures)
            {
                if(localTexture.resource.description.equivelentTo(alias.second) && !claimedAliases.contains(alias.first))
                {
                    claimedAliases.insert(alias.first);
                    aliases[localTexture.resource.name] = alias.first;
                    foundAlias = true;
                    memoryBarriers.push_back(framegraph::VirtualImageMemoryBarrier
                    {
                        //I don't care what the previous layout was, I'm throwing it away completely
                        .oldLayout = Texture::Layout::UNDEFINED,
                        .newLayout = localTexture.layout,
                        //TODO: I'm not sure if I can get away with this... it's convenient if I can, if not, I have to track it...
                        .requireCachesFinish = PipelineAccess::PipeLineAccessFlags::NONE,
                        .usingCaches = PipelineAccess::PipeLineAccessFlags::NONE,
                        .textureName = localTexture.resource.name
                    });
                    break;
                }
            }
            if(!foundAlias)
            {
                createTextures[localTexture.resource.name] = localTexture.resource.description;
            }
        }


        passes.push_back(currentPass);
    }
} // slag