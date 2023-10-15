#include "Resource.h"
#include "../Swapchain.h"
#include "ResourceManager.h"


namespace slag
{

    void Resource::smartDestroy()
    {
        if(destroyImmediately)
        {
            if(freeResources)
            {
                freeResources();
            }
        }
        else if(freeResources)
        {
            backend::ResourceManager::queueDeleteResource(this,freeResources);
        }
    }

    Resource::Resource(Resource&& from)
    {
        std::swap(destroyImmediately,from.destroyImmediately);
        std::swap(freeResources,from.freeResources);
    }

    Resource& Resource::operator=(Resource&& from)
    {
        std::swap(destroyImmediately,from.destroyImmediately);
        std::swap(freeResources,from.freeResources);
        return *this;
    }

}