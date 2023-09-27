#include "ResourceManager.h"

namespace slag
{
    namespace backend
    {
        boost::uuids::random_generator generator;
        void ResourceManager::queueDeleteResource(Resource *resource, std::function<void()>& deletion)
        {
            std::lock_guard<std::mutex> swapchainGuard(_activeSwapchainsMutex);
            auto activeSwapchainCount = _activeSwapchains.size();
            if(activeSwapchainCount == 0)
            {
                deletion();
                return;
            }
            boost::uuids::uuid deletionId = generator();
            _deletionTracker[deletionId] = {deletion,activeSwapchainCount};
            auto i = _deletionTracker.size();
            for(auto& swapchain: _activeSwapchains)
            {
                swapchain->currentFrame()->queueResourceForDeletion(deletionId);
            }
        }

        void ResourceManager::freeResourceUsage(boost::uuids::uuid deletionUUID)
        {
            auto& element = _deletionTracker[deletionUUID];
            element.usageCount--;
            if(element.usageCount <= 0 )
            {
                element.resourceFreeFunction();
                _deletionTracker.erase(deletionUUID);
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