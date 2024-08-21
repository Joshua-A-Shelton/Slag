#ifndef SLAG_FENCE_H
#define SLAG_FENCE_H
#include <cstdint>

namespace slag
{

    class Semaphore;
    struct SemaphoreValue
    {
        Semaphore* semaphore = nullptr;
        uint64_t value = 0;
    };
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

        static void waitFor(SemaphoreValue* values, size_t count);
    };

} // slag

#endif //SLAG_FENCE_H
