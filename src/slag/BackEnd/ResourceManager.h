#ifndef SLAG_RESOURCEMANAGER_H
#define SLAG_RESOURCEMANAGER_H

#include "../Resource.h"
#include "../Swapchain.h"
#include <unordered_map>
#include <mutex>
#include <set>



namespace slag
{
    namespace backend
    {
        struct ResourceUsage
        {
        public:
            std::function<void()> resourceFreeFunction;
            size_t usageCount = 0;
        };

        class ResourceManager
        {
        public:
            static void queueDeleteResource(Resource *resource, std::function<void()>& deletion);
            static void freeResourceUsage(void* resource);
            static void addSwapchain(Swapchain* swapchain);
            static void removeSwapchain(Swapchain* swapchain);

        private:
            static inline std::unordered_map<void*,ResourceUsage> _deletionTracker;
            static inline std::mutex _activeSwapchainsMutex;
            static inline std::set<Swapchain*> _activeSwapchains;
        };

    } // slag
} // backend

#endif //SLAG_RESOURCEMANAGER_H
