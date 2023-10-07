#include "ResourceManager.h"

namespace slag
{
    namespace backend
    {
        void ResourceManager::queueDeleteResource(Resource *resource, std::function<void()>& deletion)
        {
            std::lock_guard<std::mutex> swapchainGuard(_activeSwapchainsMutex);
            auto activeSwapchainCount = _activeSwapchains.size();
            if(activeSwapchainCount == 0)
            {
                deletion();
                return;
            }
            _deletionTracker[resource->GPUID()] = {deletion,activeSwapchainCount};
            for(auto& swapchain: _activeSwapchains)
            {
                swapchain->currentFrame()->queueResourceForDeletion(resource->GPUID());
            }
        }

        void ResourceManager::freeResourceUsage(void* resource)
        {
            auto element = _deletionTracker[resource];
            element.usageCount--;
            if(element.usageCount <= 0 )
            {
                element.resourceFreeFunction();
                _deletionTracker.erase(resource);
            }
        }

        void ResourceManager::addSwapchain(Swapchain *swapchain)
        {
            std::lock_guard<std::mutex> swapchainGuard(_activeSwapchainsMutex);
            if(swapchain)
            {
                _activeSwapchains.insert(swapchain);
            }
        }

        void ResourceManager::removeSwapchain(Swapchain *swapchain)
        {
            std::lock_guard<std::mutex> swapchainGuard(_activeSwapchainsMutex);
            if(swapchain)
            {
                _activeSwapchains.erase(swapchain);
            }
        }
    } // slag
} // backend