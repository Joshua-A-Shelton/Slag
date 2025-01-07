#include "ResourceManager.h"
#include "slag/CommandBuffer.h"
#include "ResourceConsumer.h"

namespace slag
{
    namespace resources
    {
        void ResourceManager::queueResourceForDeletion(Resource* resource)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            std::lock_guard<std::mutex> resourceGuard(_resourcesMutex);
            auto id = resource->gpuID();
            _resources[id] = {.references = _activeConsumers.size(), .disposalFunction = resource->_disposeFunction};
            if(_activeConsumers.empty())
            {
                resource->_disposeFunction();
            }
            else
            {
                for (auto consumer: _activeConsumers)
                {
                    consumer->addResourceReference(id);
                }
            }
        }

        void ResourceManager::setConsumerAsActive(ResourceConsumer* buffer)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            _activeConsumers.insert(buffer);
        }

        void ResourceManager::removeConsumerFromActive(ResourceConsumer* buffer)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            _activeConsumers.erase(buffer);
        }

        void ResourceManager::removeReferences(std::unordered_set<boost::uuids::uuid,boost::hash<boost::uuids::uuid>>& references)
        {
            std::lock_guard<std::mutex> resourceGuard(_resourcesMutex);
            for(auto& reference: references)
            {
                auto& data = _resources[reference];
                data.references--;
                if(data.references==0)
                {
                    data.disposalFunction();
                    _resources.erase(reference);
                }
            }
        }
    } // resources
} // slag