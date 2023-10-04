#include "Frame.h"
#include "BackEnd/ResourceManager.h"
namespace slag
{
    Frame::~Frame()
    {
        freeResourceReferences();
    }

    void Frame::queueResourceForDeletion(void* gpuID)
    {
        std::lock_guard<std::mutex> swapchainGuard(_freedResourcesMutex);
        _freedResourceReferences.push_back(gpuID);
    }

    void Frame::freeResourceReferences()
    {
        std::lock_guard<std::mutex> swapchainGuard(_freedResourcesMutex);
        for(auto i=0; i< _freedResourceReferences.size(); i++)
        {
            backend::ResourceManager::freeResourceUsage(_freedResourceReferences[i]);
        }
        _freedResourceReferences.clear();
    }



}