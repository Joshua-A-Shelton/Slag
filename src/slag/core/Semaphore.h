#ifndef SLAG_SEMAPHORE_H
#define SLAG_SEMAPHORE_H
#include <cstdint>

namespace slag
{
    ///Synchronization primitive that increases a counter to indicate when events have occurred on the GPU
    class Semaphore
    {
    public:
        virtual ~Semaphore();
        ///Current counter value of the semaphore
        virtual uint64_t value()=0;
        /**
         * Set the value of the semaphore from the CPU
         * @param value New value of the semaphore
         */
        virtual void signal(uint64_t value)=0;
        /**
         * Wait on the CPU until the given value is reached
         * @param value Value for the semaphore to reach that ends the wait
         */
        virtual void waitForValue(uint64_t value)=0;
    };

    struct SemaphoreValue
    {
        Semaphore* semaphore=nullptr;
        uint64_t value=0;
    };
} // slag

#endif //SLAG_SEMAPHORE_H
