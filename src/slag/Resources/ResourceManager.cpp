#include "ResourceManager.h"
#include "slag/CommandBuffer.h"

namespace slag
{
    namespace resources
    {
        void ResourceManager::queueResourceForDeletion(Resource* resource)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            std::lock_guard<std::mutex> resourceGuard(_resourcesMutex);
            auto id = resource->gpuID();
            _resources[id] = {.references = _activeCommandBuffers.size(), .disposalFunction = resource->_disposeFunction};
            if(_activeCommandBuffers.empty())
            {
                resource->_disposeFunction();
            }
            else
            {
                for (auto commandBuffer: _activeCommandBuffers)
                {
                    commandBuffer->addResourceReference(id);
                }
            }
        }

        void ResourceManager::setBufferAsActive(CommandBuffer* buffer)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            _activeCommandBuffers.insert(buffer);
        }

        void ResourceManager::removeBufferFromActive(CommandBuffer* buffer)
        {
            std::lock_guard<std::mutex> bufferGuard(_activeCommandMutex);
            _activeCommandBuffers.erase(buffer);
        }

        void ResourceManager::removeReferences(std::unordered_set<void*>& references)
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