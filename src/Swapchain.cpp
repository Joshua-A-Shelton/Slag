#include "Swapchain.h"
#include "Resource.h"
namespace slag
{

    Swapchain::Swapchain()
    {
        std::lock_guard<std::mutex> guard(_activeSwapchainsMutex);
        _activeSwapchains.insert(this);
    }

    Swapchain::~Swapchain()
    {
        std::lock_guard<std::mutex> guard(_activeSwapchainsMutex);
        _activeSwapchains.erase(this);
    }

    const std::set<Swapchain *> &Swapchain::activeSwapchains()
    {
        return _activeSwapchains;
    }

}