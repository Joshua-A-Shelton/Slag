#include "FrameResourceDictionary.h"

namespace slag
{
    void FrameResourceDictionary::assignGlobalTexture(std::string name, Texture *texture)
    {
        _textures[name] = texture;
    }

    void FrameResourceDictionary::assignTransientTexture(std::string name, Texture *texture)
    {
        _textures[name] = texture;
        _transientTextures.push_back(texture);
    }

    void FrameResourceDictionary::destroyTransients()
    {
        for(auto texture: _transientTextures)
        {
            delete texture;
        }
        _transientTextures.clear();
    }

    Texture* FrameResourceDictionary::getTexture(std::string textureName)
    {
        return _textures.at(textureName);
    }
} // slag