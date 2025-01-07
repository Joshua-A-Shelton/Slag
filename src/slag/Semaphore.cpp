#include "Semaphore.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    Semaphore* Semaphore::newSemaphore(uint64_t startingValue)
    {
        return lib::BackEndLib::get()->newSemaphore(startingValue);
    }

    void Semaphore::waitFor(SemaphoreValue* values, size_t count)
    {
        lib::BackEndLib::get()->waitFor(values,count);
    }
} // slag