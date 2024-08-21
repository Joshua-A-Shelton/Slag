#ifndef SLAG_GPUQUEUE_H
#define SLAG_GPUQUEUE_H
#include "CommandBuffer.h"
#include "Semaphore.h"
namespace slag
{

    class GpuQueue
    {
    public:
        enum QueueType
        {
            Graphics,
            Transfer,
            Compute,
        };
        virtual ~GpuQueue()=default;
        virtual void submit(CommandBuffer* commands)=0;
        /**
         *
         * @param commandBuffers
         * @param bufferCount
         * @param forceDependency require each submitted command buffer to finish before starting the next one. If false, the entire submission is treated as if it was a single command buffer
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, bool forceDependency)=0;
        virtual void submit(CommandBuffer* commands, SemaphoreValue& signalFinished)=0;
        /**
         *
         * @param commandBuffers
         * @param bufferCount
         * @param signalFinished
         * @param forceDependency require each submitted command buffer to finish before starting the next one. If false, the entire submission is treated as if it was a single command buffer
         */
        virtual void submit(CommandBuffer** commandBuffers, size_t bufferCount, SemaphoreValue& signalFinished, bool forceDependency)=0;
        virtual QueueType type()=0;
    };

} // slag

#endif //SLAG_GPUQUEUE_H
