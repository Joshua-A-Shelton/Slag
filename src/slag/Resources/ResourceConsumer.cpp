#include "ResourceConsumer.h"
#include "ResourceManager.h"
namespace slag
{
    namespace resources
    {
        ResourceConsumer::~ResourceConsumer()
        {
            freeResourceReferences();
        }

        void ResourceConsumer::freeResourceReferences()
        {
            std::lock_guard<std::mutex> resourceLock(_referencesMutex);
            resources::ResourceManager::removeReferences(_resourceReferences);
            _resourceReferences.clear();
        }

        void ResourceConsumer::addResourceReference(boost::uuids::uuid gpuID)
        {
            std::lock_guard<std::mutex> resourceLock(_referencesMutex);
            _resourceReferences.insert(gpuID);
        }

        void ResourceConsumer::move(ResourceConsumer& from)
        {
            std::swap(_resourceReferences,from._resourceReferences);
        }
    } // resources
} // slag