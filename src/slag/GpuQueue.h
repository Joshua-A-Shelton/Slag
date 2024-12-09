#ifndef SLAG_GPUQUEUE_H
#define SLAG_GPUQUEUE_H

#include "Semaphore.h"
#include "Frame.h"

namespace slag
{

    class CommandBuffer;
    ///GPU object that can execute command buffers
    class GpuQueue
    {
    public:
        ///What kind of commands can be processed
        enum QueueType
        {
            ///Can process Graphics commands, Compute commands, and Transfer commands
            GRAPHICS,
            ///Can process Transfer commands
            TRANSFER,
            ///Can process Compute and Transfer Commands
            COMPUTE,
        };
        virtual ~GpuQueue()=default;
        ///Execute commands recorded in command buffer
        virtual void submit(CommandBuffer* commands)=0;
        /**
         * Execute commands recorded in command buffers
         * @param commandBuffers Array of command buffers to execute
         * @param bufferCount Number of command buffers to execute
         * @param forceDependency Require each submitted command buffer to finish before starting the next one. If false, the entire submission is treated as if it was a single command buffer
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)=0;
        /**
         * Execute commands recorded in command buffers
         * @param commands command buffer to execute
         * @param signalFinished Semaphore signal operation that occurs after command buffer is finished
         */
        virtual void submit(CommandBuffer* commands, SemaphoreValue& signalFinished)=0;
        /**
         * Execute commands recorded in command buffers
         * @param commandBuffers Array of command buffers to execute
         * @param bufferCount Number of command buffers to execute
         * @param signalFinished Semaphore signal operation that occurs after command buffers are finished
         * @param forceDependency Require each submitted command buffer to finish before starting the next one. If false, the entire submission is treated as if it was a single command buffer
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished, bool forceDependency)=0;
        /**
         * Execute commands recorded in command buffers
         * @param commandBuffers Array of command buffers to execute
         * @param bufferCount Number of command buffers to execute
         * @param waitOnSemaphores Array of Semaphore signal requirements to finish before executing this batch of command buffers
         * @param waitCount Number of wait semaphore data
         * @param signalSemaphores Array of Semaphore signal operations that occurs after command buffers are finished
         * @param signalCount Number of signal semaphore data
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount)=0;

        /**
         * Execute commands recorded in command buffers, and signal frame for presenting to the screen
         * @param commandBuffers Array of command buffers to execute
         * @param bufferCount Number of command buffers to execute
         * @param waitOnSemaphores Array of Semaphore signal requirements to finish before executing this batch of command buffers
         * @param waitCount Number of wait semaphore data
         * @param signalSemaphores Array of Semaphore signal operations that occurs after command buffers are finished
         * @param signalCount Array of Semaphore signal operations that occurs after command buffers are finished
         * @param presentFrame Frame to present to the screen after commands are finished
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue* waitOnSemaphores, size_t waitCount, SemaphoreValue* signalSemaphores, size_t signalCount, Frame* presentFrame)=0;

        ///The type of commands this queue can process
        virtual QueueType type()=0;
    };

} // slag

#endif //SLAG_GPUQUEUE_H
