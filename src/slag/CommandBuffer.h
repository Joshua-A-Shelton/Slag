#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H

#include <mutex>
#include <unordered_set>
#include "slag/Resources/ResourceManager.h"
#include "Semaphore.h"
#include "GpuMemoryBarriers.h"
#include "GpuQueue.h"
#include "Texture.h"
#include "Clear.h"

namespace slag
{

    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer();
        friend class slag::resources::ResourceManager;
        virtual void begin()=0;
        virtual void end()=0;
        virtual void waitUntilFinished()=0;
        virtual bool isFinished()=0;
        virtual GpuQueue::QueueType commandType()=0;

        static CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands);

        virtual void insertBarriers(ImageBarrier* imageBarriers, size_t imageBarrierCount, BufferBarrier* bufferBarriers, size_t bufferBarrierCount)=0;
        virtual void clearColorImage(Texture* texture, ClearColor color, Texture::Layout layout)=0;

    protected:
        void freeResourceReferences();
    private:
        std::mutex _referencesMutex;
        std::unordered_set<void*> _resourceReferences;
        void addResourceReference(void* gpuID);

    };

} // slag

#endif //CRUCIBLEEDITOR_COMMANDBUFFER_H
