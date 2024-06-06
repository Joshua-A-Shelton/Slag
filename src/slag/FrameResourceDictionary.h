#ifndef SLAG_FRAMERESOURCEDICTIONARY_H
#define SLAG_FRAMERESOURCEDICTIONARY_H
#include <vector>
#include <unordered_map>
#include <string>
#include "Texture.h"
namespace slag
{

    class FrameResourceDictionary
    {
    public:
        Texture* getTexture(std::string textureName);
        friend class FrameGraph;
        friend class FrameGraphStage;
    private:
        void assignGlobalTexture(std::string name, Texture* texture);
        void assignTransientTexture(std::string name, Texture* texture);
        void destroyTransients();
        std::unordered_map<std::string,Texture*> _textures;
        std::vector<Texture*> _transientTextures;

    };

} // slag

#endif //CRUCIBLEEDITOR_FRAMERESOURCEDICTIONARY_H
