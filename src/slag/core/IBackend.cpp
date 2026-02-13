#include "IBackend.h"
#include <stdexcept>
#include <slag/exceptions/NotImplemented.h>

namespace slag
{
    BackendAPI IBackend::api()const
    {
        throw NotImplemented();
    }

    uint32_t IBackend::graphicsCardCount()const
    {
        throw NotImplemented();
    }

    GraphicsCard* IBackend::graphicsCard(uint32_t index)
    {
        throw NotImplemented();
    }

    SlagInitializationResult IBackend::initializeBackend(const InitializationData& initializationData)
    {
        throw NotImplemented();
    }
} // slag
