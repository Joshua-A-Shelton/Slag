#ifndef SLAG_GRAPHICSCARD_H
#define SLAG_GRAPHICSCARD_H
#include <string>
#include "Buffer.h"
#include "CommandBuffer.h"
#include "SubmissionQueue.h"

namespace slag
{
    class GraphicsCard
    {
    protected:
        GraphicsCard() = default;
    public:
        virtual ~GraphicsCard() = default;
        ///Name of Graphics Card
        [[nodiscard]] virtual std::string name()const=0;
        ///Total size in bytes of video memory
        [[nodiscard]] virtual uint64_t videoMemory()const=0;
        ///If the entire range of GPU memory is accessible by the CPU (true if under Unified Memory Access, or resizeable BAR is enabled)
        [[nodiscard]] virtual bool cacheCoherentSharedMemory()const=0;
        ///The largest possible size in bytes of a buffer with BufferShaderAccess::READ_ONLY
        [[nodiscard]] virtual uint64_t maxShaderAccessReadOnlyBufferSize()const=0;

        ///Queue that supports graphics operations
        [[nodiscard]] virtual SubmissionQueue* graphicsQueue()=0;
        ///Queue that supports compute operations (dedicated if supported, will fall back to a higher queue if not)
        [[nodiscard]] virtual SubmissionQueue* computeQueue()=0;
        ///Queue that supports transfer operations (dedicated if supported, will fall back to a higher queue if not)
        [[nodiscard]] virtual SubmissionQueue* transferQueue()=0;

        /**
         * Defragment video memory. This is a blocking call, and will not return until defragmentation is complete
         * @param waitFor Array of SemaphoreValues to wait for before defragmentation begins
         * @param waitCount Number of SemaphoreValues in waitFor array
         * @param signal Array of SemaphoreValues to signal after defragmentation ends
         * @param signalCount Number of SemaphoreValues in signal array
         * @param targetBytes Number of bytes to attempt to defragment before finishing, or 0 for a full defragmenation
         * @return number of bytes defragmented
         */
        virtual uint64_t defragmentMemory(SemaphoreValue* waitFor, uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount, uint64_t targetBytes=0)=0;

        //Command Buffers
        /**
         * Create a new command buffer
         * @param type What kind of operations this buffer can perform
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new command buffer
         */
        [[nodiscard]] virtual CommandBuffer* newCommandBuffer(QueueType type)=0;

        //Semaphores
        /**
         * Create a new Semaphore
         * @param initialValue initial value of the semaphore
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new command buffer
         */
        [[nodiscard]] virtual Semaphore* newSemaphore(uint64_t initialValue=0)=0;

        //Buffers
        /**
         * Allocate a new buffer
         * @param size Size in bytes of the buffer
         * @param usage Kind of data will the buffer store
         * @param shaderAccess Data access permissions for shaders
         * @param cpuAccess Data access permissions for the cpu
         * @return
         * @throws slag::ResourceCreationError if unable to allocate a new command buffer
         */
        [[nodiscard]] virtual Buffer* newBuffer(
            uint64_t size,
            BufferUsage usage = BufferUsage::ARBITRARY,
            BufferShaderAccess shaderAccess = BufferShaderAccess::READ_WRITE,
            BufferCPUAccess cpuAccess = BufferCPUAccess::NONE)=0;
    };
} // slag

#endif //SLAG_GRAPHICSCARD_H
