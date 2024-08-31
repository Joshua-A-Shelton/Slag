#ifndef SLAG_FRAME_H
#define SLAG_FRAME_H

#include "Texture.h"
#include "CommandBuffer.h"

namespace slag
{

    class Frame
    {
    public:
        virtual ~Frame()=default;
        virtual Texture* backBuffer()=0;
        virtual CommandBuffer* commandBuffer()=0;
        virtual void present()=0;

    };

} // slag

#endif //SLAG_FRAME_H
