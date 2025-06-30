#ifndef SLAG_SEMAPHORE_H
#define SLAG_SEMAPHORE_H
#include <cstdint>

namespace slag
{
    class Semaphore;
    ///Coupling of a semaphore and a value for that semaphore
    struct SemaphoreValue
    {
        Semaphore* semaphore=nullptr;
        uint32_t value=0;
    };
    ///Synchronization primitive that increases a counter to indicate when events have occurred on the GPU
    class Semaphore
    {
    public:
        virtual ~Semaphore()=default;
        ///Current counter value of the semaphore
        virtual uint64_t value()=0;

        /**
         * Set the value of a semaphore from the CPU
         * @param value new value for the semaphore
         */
        virtual void signal(uint64_t value)=0;

        /**
         * 
         * @param value counter value to which to stall the CPU until it is reached
         */
        virtual void waitForValue(uint64_t value)=0;

        static Semaphore* newSemaphore(uint64_t startingValue = 0);

        /**
         * Stall CPU until all semaphores reach certain values
         * @param values array of semaphore/values to wait on
         * @param count the number of semaphores in the array to wait for
         */
        static void waitFor(SemaphoreValue* values, size_t count);

    };
} // slag

#endif //SLAG_SEMAPHORE_H
