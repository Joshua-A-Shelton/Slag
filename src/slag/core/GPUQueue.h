#ifndef SLAG_GPUQUEUE_H
#define SLAG_GPUQUEUE_H
#include "Semaphore.h"

namespace slag
{
    class CommandBuffer;
    class Frame;

    ///Data to be sent to GPU for submission
    struct QueueSubmissionBatch
    {
        ///Semaphores to wait on before executing this batch of command buffers
        SemaphoreValue* waitSemaphores = nullptr;
        ///Number of semaphore values in waitSemaphores
        uint32_t waitSemaphoreCount = 0;
        ///Command buffers to execute
        CommandBuffer** commandBuffers = nullptr;
        ///Number of command buffers
        uint32_t commandBufferCount = 0;
        ///Semaphores to signal after executing this batch of command buffers
        SemaphoreValue* signalSemaphores = nullptr;
        ///Number of semaphore values in signalSemaphores
        uint32_t signalSemaphoreCount =0;
    };

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
         * @param submissionData Information to submit to the queue
         * @param submissionDataCount Number of Submission Data
         */
        virtual void submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount)=0;
        /**
         * Submit command buffers for execution and frame for presentation
         * @param submissionData Information to submit to the queue
         * @param submissionDataCount Number of Submission Data
         * @param frame The frame to present after the buffers execute
         */
        virtual void submit(QueueSubmissionBatch* submissionData, uint32_t submissionDataCount,Frame* frame)=0;

        /**
         * If a queue can process commands of a given type
         * @param queue
         * @param commandBuffer
         * @return
         */
        static bool canProcessCommands(QueueType queue, QueueType commandBuffer)
        {
            switch (queue)
            {
                case QueueType::GRAPHICS:
                    return true;
                case QueueType::COMPUTE:
                    if (commandBuffer != QueueType::GRAPHICS)
                    {
                        return true;
                    }
                    return false;
                case QueueType::TRANSFER:
                    if (commandBuffer == QueueType::TRANSFER)
                    {
                        return true;
                    }
                return false;
            }
            return false;
        }

    };
} // slag

#endif //SLAG_GPUQUEUE_H
