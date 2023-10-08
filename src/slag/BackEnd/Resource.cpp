#include "Resource.h"
#include "../Swapchain.h"
#include "ResourceManager.h"


namespace slag
{

    void Resource::destroyDeferred()
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

}