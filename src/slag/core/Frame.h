#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include <concepts>
#include "FrameResources.h"
#include "Semaphore.h"

namespace slag
{
    class Texture;
    class Semaphore;
    class Frame
    {
    public:
        virtual ~Frame();
        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;
        Frame(Frame&& from);
        Frame& operator=(Frame&& from);
        ///Get the texture for this frame, there is no guarantee a frame will have the same texture between submissions to GPUQueue::submit()
        virtual Texture* backBuffer()=0;
        ///Get the index for this frame
        virtual uint8_t frameIndex()=0;

    protected:
        FrameResources* getFrameResources();
        Frame(FrameResources* resources);
        FrameResources* _resources = nullptr;
        void move(Frame& from);
    public:
        ///Get user defined frame resources
        template <std::derived_from<FrameResources> T> T* frameResources()
        {
            return static_cast<T*>(this->getFrameResources());
        }


    };
} // slag

#endif //SLAG_FRAME_H
