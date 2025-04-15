#ifndef SLAG_RESOURCEMANAGER_H
#define SLAG_RESOURCEMANAGER_H

#include <unordered_set>
#include <functional>
#include <mutex>
#include "Resource.h"
#include <boost/container_hash/hash.hpp>

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
            static void queueResourceForDeletion(boost::uuids::uuid id, const std::function<void()>& disposeNonAllocations);
            static void setConsumerAsActive(ResourceConsumer* buffer);
            static void removeConsumerFromActive(ResourceConsumer* buffer);
            friend class ResourceConsumer;
        private:
            static inline std::mutex _activeCommandMutex;
            static inline std::unordered_set<ResourceConsumer*> _activeConsumers;
            static inline std::mutex _resourcesMutex;
            static inline std::unordered_map<boost::uuids::uuid,ResourceState,boost::hash<boost::uuids::uuid>> _resources;
            static void removeReferences(std::unordered_set<boost::uuids::uuid,boost::hash<boost::uuids::uuid>>& references);

        };

    } // resources
} // slag

#endif //CRUCIBLEEDITOR_RESOURCEMANAGER_H
