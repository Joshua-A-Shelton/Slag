#ifndef SLAG_GRAPHICSPASS_H
#define SLAG_GRAPHICSPASS_H
#include "FrameResourceDescriptions.h"
#include "FrameResourceDictionary.h"
#include "CommandBuffer.h"
#include <string>
#include <vector>
namespace slag
{

    class GraphicsPass
    {
    public:
        GraphicsPass(std::string name, PipelineStage::PipelineStageFlags usage);
        ~GraphicsPass()=default;
        GraphicsPass(const GraphicsPass& from);
        GraphicsPass& operator=(const GraphicsPass& from);
        GraphicsPass(GraphicsPass&& from);
        GraphicsPass& operator=(GraphicsPass&& from);
        //void addResource(TextureResourceDescription resource,FrameResourceDescriptions::Usage usage);
        GraphicsPass& addLocalTexture(const std::string& name, TextureResourceDescription description, Texture::Layout layout);
        GraphicsPass& addGlobalTexture(const std::string& name,Texture::Usage textureType, Texture::Layout layout, PipelineAccess::PipeLineAccessFlags textureUsage);
        void setCallback(void (*callback)(CommandBuffer* commandBuffer, FrameResourceDictionary& resources, void* additionalData));
        friend class FrameGraphBuilder;
        friend class FrameGraph;
    private:
        std::string _name;
        PipelineStage::PipelineStageFlags _stageUsage;
        std::vector<TextureResourceUsage> _localTextures;
        std::unordered_map<std::string, GlobablTextureUsage> _globalUsages;
        void (*_callback)(CommandBuffer* commandBuffer, FrameResourceDictionary& resources, void* additionalData) = nullptr;

        void move(GraphicsPass&& from);
        void copy(const GraphicsPass& from);

    };

} // slag

#endif //CRUCIBLEEDITOR_GRAPHICSPASS_H
