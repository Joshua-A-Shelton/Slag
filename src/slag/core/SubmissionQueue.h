#ifndef SLAG_SUBMITQUEUE_H
#define SLAG_SUBMITQUEUE_H
#include <cstdint>

#include "Semaphore.h"

namespace slag
{
    class CommandBuffer;
    ///Describes what kind of operations the queue or command buffer can support
    enum class QueueType
    {
        ///Supports all operation types
        GRAPHICS,
        ///Supports Compute and Transfer operation types
        COMPUTE,
        ///Supports transfer operation types
        TRANSFER
    };
    ///A group of work to be executed on the graphics card
    struct SubmissionBatch
    {
        ///Array of SemaphoreValues to wait for before the command buffers start execution, nullptr if there are none
        SemaphoreValue* waitSemaphores = nullptr;
        ///Number of SemaphoreValues in waitSemaphores array
        uint32_t waitSemaphoreCount = 0;
        ///Array of CommandBuffers to execute
        CommandBuffer** commandBuffers = nullptr;
        ///Number of CommandBuffers in commandBuffers Array
        uint32_t commandBufferCount = 0;
        ///Array of SemaphoreValues to signal after command buffers finish execution, nullptr if there are none
        SemaphoreValue* signalSemaphores = nullptr;
        ///Number of SemaphoreValues in signalSemaphores array
        uint32_t signalSemaphoreCount = 0;
    };
    ///Executes command buffers
    class SubmissionQueue
    {
    public:
        virtual ~SubmissionQueue() = default;
        ///What kind of operations can this queue process
        [[nodiscard]] virtual QueueType type()const=0;
        ///Which graphics card this buffer belongs to
        [[nodiscard]] virtual GraphicsCard* graphicsCard()=0;
        /**
         * Submit commands for execution
         * @param batch Batch of commands to execute
         */
        virtual void submit(const SubmissionBatch& batch)=0;
        /**
         * Submit multiple batches of commands for execution
         * @param batches Array of batches to execute
         * @param batchCount Number of batches in batches array
         */
        virtual void submit(SubmissionBatch* batches, uint32_t batchCount)=0;
    };

    inline bool QueueTypeSupportsCommands(QueueType queue, QueueType commands)
    {
        if (queue == QueueType::GRAPHICS)
        {
            return true;
        }
        else if (queue == QueueType::COMPUTE)
        {
            if (commands == QueueType::GRAPHICS)
            {
                return false;
            }
            return true;
        }
        else
        {
            if (commands == QueueType::TRANSFER)
            {
                return true;
            }
            return false;
        }

    }
} // slag

#endif //SLAG_SUBMITQUEUE_H
