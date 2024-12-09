#ifndef SLAG_FENCE_H
#define SLAG_FENCE_H
#include <cstdint>
#include <cstddef>

namespace slag
{

    class Semaphore;
    ///Coupling of a semaphore and a value for that semaphore
    struct SemaphoreValue
    {
        Semaphore* semaphore = nullptr;
        uint64_t value = 0;
    };
    ///Synchronization primitive that increases a counter to indicate when events have occured on the GPU
    class Semaphore
    {
    public:
        virtual ~Semaphore()=default;
        virtual uint64_t value()=0;
        virtual void signal(uint64_t value)=0;
        /**
         * stalls cpu until the semaphore reaches the given value, can be safely used across different threads without stalling other threads
         * @param value
         */
        virtual void waitForValue(uint64_t value)=0;

        static Semaphore* newSemaphore(uint64_t startingValue = 0);
        /**
         * Wait for all semaphores to reacha value before returning
         * @param values
         * @param count
         */
        static void waitFor(SemaphoreValue* values, size_t count);
    };

} // slag

#endif //SLAG_FENCE_H
