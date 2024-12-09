#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H

namespace slag
{

    class Texture;
    class CommandBuffer;
    ///Resources used for each instance of the swapchain::next
    class Frame
    {
    public:
        virtual ~Frame()=default;
        virtual Texture* backBuffer()=0;
        virtual CommandBuffer* commandBuffer()=0;

    };

} // slag

#endif //SLAG_FRAME_H
