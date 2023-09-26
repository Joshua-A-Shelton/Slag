#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include <vector>
#include <mutex>
#include <boost/uuid/uuid.hpp>
#include "CommandBuffer.h"

namespace slag
{
    class Frame
    {
    public:
        virtual ~Frame();
        void begin();
        void end();
        virtual CommandBuffer* getCommandBuffer()=0;
        ///Internal use only, automatically called for resources
        void queueResourceForDeletion(boost::uuids::uuid resourceID);
    protected:
        virtual void beginFrame()=0;
        virtual void endFrame()=0;
    private:
        void freeResourceReferences();
        std::mutex _freedResourcesMutex;
        std::vector<boost::uuids::uuid> _freedResourceReferences;
    };
}
#endif //SLAG_FRAME_H
