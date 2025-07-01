#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include <concepts>
#include "FrameResources.h"

namespace slag
{
    class Texture;
    class Semaphore;
    class Frame
    {
    public:
        virtual ~Frame()=default;
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
        Frame(Frame&& from);
        Frame& operator=(Frame&& from);
        virtual Texture* backBuffer()=0;
        virtual Semaphore* finishedSemaphore()=0;
    protected:
        virtual FrameResources* getFrameResources()=0;
    public:
        template <std::derived_from<FrameResources> T> T* frameResources()
        {
            return static_cast<T*>(this->getFrameResources());
        }


    };
} // slag

#endif //SLAG_FRAME_H
