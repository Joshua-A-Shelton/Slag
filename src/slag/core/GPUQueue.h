#ifndef SLAG_GPUQUEUE_H
#define SLAG_GPUQUEUE_H
#include "Semaphore.h"

namespace slag
{
    class CommandBuffer;
    class Frame;
    ///Graphics card queue that accepts command buffers and executes the commands in them
    class GPUQueue
    {
    public:
        enum class QueueType
        {
            ///Can process Graphics commands, Compute commands, and Transfer commands
            GRAPHICS,
            ///Can process Compute and Transfer commands
            COMPUTE,
            ///Can process Transfer commands
            TRANSFER
        };
        virtual ~GPUQueue()=default;
        ///The actual type this queue is, as lower queues commands can be processed by higher queues
        virtual QueueType type()=0;

        /**
         * Submit command buffers for execution
         * @param commandBuffers Command buffers to execute
         * @param commandBufferCount Number of command buffers
         * @param waitSemaphores Semaphores to wait on before executing submitted command buffers
         * @param waitSemaphoreCount Number of semaphores to wait on
         * @param signalSemaphores Semaphores to signal after execution
         * @param signalSemaphoreCount Number of semaphores to signal
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores, size_t signalSemaphoreCount)=0;

        /**
         * Submit a frame for execution and display
         * @param frame Frame to submit for execution
         * @param commandBuffers Command buffers to execute
         * @param commandBufferCount Number of command buffers
         * @param waitSemaphores Semaphores to wait on before executing submitted command buffers
         * @param waitSemaphoreCount Number of semaphores to wait on
         * @param signalSemaphores Semaphores to signal after execution
         * @param signalSemaphoreCount Number of semaphores to signal
         */
        virtual void submit(Frame* frame,CommandBuffer** commandBuffers, size_t commandBufferCount, SemaphoreValue* waitSemaphores, size_t waitSemaphoreCount, SemaphoreValue* signalSemaphores, size_t signalSemaphoreCount)=0;

    };
} // slag

#endif //SLAG_GPUQUEUE_H
