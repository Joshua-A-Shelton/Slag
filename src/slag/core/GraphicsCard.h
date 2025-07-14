#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include "GPUQueue.h"
#include <cstdint>
#include <string>

namespace slag
{
    ///Represents hardware that performs parallel commands
    class GraphicsCard
    {
    public:
        virtual ~GraphicsCard() = default;
        ///Total video memory in bytes
        virtual uint64_t videoMemory()=0;
        ///Name of graphics card
        virtual std::string name()=0;

        ///Gets the graphics queue
        virtual GPUQueue* graphicsQueue()=0;
        ///Gets the compute queue, or a default queue that processes it's commands
        virtual GPUQueue* computeQueue()=0;
        ///Gets the transfer queue, or a queue that processes it's commands
        virtual GPUQueue* transferQueue()=0;

        ///Total number of graphics queues (minimum 1 guaranteed)
        virtual uint32_t graphicsQueueCount()=0;
        ///Total number of compute queues
        virtual uint32_t computeQueueCount()=0;
        ///Total number of transfer queues
        virtual uint32_t transferQueueCount()=0;

        /**
         * Gets the graphics queue at a given index
         * @param index
         * @return
         */
        virtual GPUQueue* graphicsQueue(uint32_t index)=0;

        /**
         * Gets a dedicated compute queue at a given index
         * @param index
         * @return
         */
        virtual GPUQueue* computeQueue(uint32_t index)=0;

        /**
         * Gets a dedicated transfer queue at a given index
         * @param index
         * @return
         */
        virtual GPUQueue* transferQueue(uint32_t index)=0;

        ///Defragment video memory, blocks until finished
        virtual void defragmentMemory(SemaphoreValue* waitFor, size_t waitForCount, SemaphoreValue* signal, size_t signalCount)=0;
    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
