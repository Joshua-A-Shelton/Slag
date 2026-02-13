#ifndef SLAG_SUBMITQUEUE_H
#define SLAG_SUBMITQUEUE_H
#include <cstdint>

#include "CommandBuffer.h"
#include "Semaphore.h"

namespace slag
{
    ///Describes what kind of operations the queue or command buffer can support
    enum QueueType
    {
        ///Supports all operation types
        GRAPHICS,
        ///Supports Compute and Transfer operation types
        COMPUTE,
        ///Supports transfer operation types
        TRANSFER
    };

    struct SubmissionBatch
    {
        SemaphoreValue* waitSemaphores = nullptr;
        uint32_t waitSemaphoreCount = 0;
        CommandBuffer** commandBuffers = nullptr;
        uint32_t commandBufferCount = 0;
        SemaphoreValue* signalSemaphores = nullptr;
        uint32_t signalSemaphoreCount = 0;
    };

    class SubmissionQueue
    {
    public:
        virtual ~SubmissionQueue() = default;
        virtual QueueType type()=0;
        virtual void submit(SubmissionBatch* batches, uint32_t batchCount)=0;
    };
} // slag

#endif //SLAG_SUBMITQUEUE_H
