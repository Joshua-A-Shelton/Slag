#include "Semaphore.h"
#include <slag/backends/Backend.h>
#include <slag/utilities/SLAG_ASSERT.h>

#include <stdexcept>

namespace slag
{
    Semaphore* Semaphore::newSemaphore(uint64_t startingValue)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        return Backend::current()->newSemaphore(startingValue);
    }

    void Semaphore::waitFor(SemaphoreValue* values, size_t count)
    {
        SLAG_ASSERT(Backend::current()!=nullptr);
        Backend::current()->waitFor(values,count);
    }
} // slag
