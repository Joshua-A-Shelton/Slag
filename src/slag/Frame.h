#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include <vector>
#include <mutex>
#include "CommandBuffer.h"

namespace slag
{
    class Frame
    {
    public:
        virtual ~Frame();
        virtual void begin()=0;
        virtual void end()=0;
        virtual CommandBuffer* getCommandBuffer()=0;
        virtual Texture* getBackBuffer()=0;
        ///Internal use only, automatically called for resources
        void queueResourceForDeletion(void* gpuID);
    protected:
        void freeResourceReferences();
    private:
        std::mutex _freedResourcesMutex;
        std::vector<void*> _freedResourceReferences;
    };
}
#endif //SLAG_FRAME_H
