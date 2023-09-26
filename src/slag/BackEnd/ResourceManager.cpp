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
            boost::uuids::uuid deletionId;
            _deletionTracker[deletionId] = {deletion,activeSwapchainCount};
            for(auto& swapchain: _activeSwapchains)
            {
                swapchain->currentFrame()->queueResourceForDeletion(deletionId);
            }
        }

        void ResourceManager::freeResourceUsage(boost::uuids::uuid deletionUUID)
        {

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