#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H
#include "FrameResources.h"
namespace slag
{

    class Texture;
    class CommandBuffer;
    ///Resources used for each instance of the swapchain::next
    class Frame
    {
    public:
        Frame(FrameResources* resources);
        virtual ~Frame();
        Frame(const Frame&)=delete;
        Frame& operator=(const Frame&)=delete;
        Frame(Frame&& from);
        Frame& operator=(Frame&& from);
        virtual Texture* backBuffer()=0;
        virtual CommandBuffer* commandBuffer()=0;
        FrameResources* resources = nullptr;
    protected:
        void move(Frame& from);
    };

} // slag

#endif //SLAG_FRAME_H
