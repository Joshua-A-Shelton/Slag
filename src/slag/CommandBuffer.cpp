#include "CommandBuffer.h"

namespace slag
{

    CommandBuffer::~CommandBuffer()
    {
        freeResourceReferences();
    }

    void CommandBuffer::freeResourceReferences()
    {
        std::lock_guard<std::mutex> resourceLock(_referencesMutex);
        resources::ResourceManager::removeReferences(_resourceReferences);
        _resourceReferences.clear();
    }

    void CommandBuffer::addResourceReference(void* gpuID)
    {
        std::lock_guard<std::mutex> resourceLock(_referencesMutex);
        _resourceReferences.insert(gpuID);
    }

} // slag