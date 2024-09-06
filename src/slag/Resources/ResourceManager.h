#ifndef SLAG_RESOURCEMANAGER_H
#define SLAG_RESOURCEMANAGER_H

#include <unordered_set>
#include <functional>
#include <mutex>
#include "Resource.h"

namespace slag
{
    namespace resources
    {
        class ResourceConsumer;

        struct ResourceState
        {
            size_t references=0;
            std::function<void()> disposalFunction;
        };
        class ResourceManager
        {
        public:
            static void queueResourceForDeletion(Resource* resource);
            static void setConsumerAsActive(ResourceConsumer* buffer);
            static void removeConsumerFromActive(ResourceConsumer* buffer);
            friend class ResourceConsumer;
        private:
            static inline std::mutex _activeCommandMutex;
            static inline std::unordered_set<ResourceConsumer*> _activeConsumers;
            static inline std::mutex _resourcesMutex;
            static inline std::unordered_map<void*,ResourceState> _resources;
            static void removeReferences(std::unordered_set<void*>& references);

        };

    } // resources
} // slag

#endif //CRUCIBLEEDITOR_RESOURCEMANAGER_H
