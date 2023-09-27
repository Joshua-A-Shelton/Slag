#ifndef SLAG_COMMANDBUFFER_H
#define SLAG_COMMANDBUFFER_H
#include "Texture.h"

namespace slag
{
    class CommandBuffer
    {
    public:
        virtual ~CommandBuffer()=default;
        virtual void transitionImageLayout(Texture* texture, Texture::Layout toLayout)=0;
        virtual void transitionImageLayout(Texture* texture, Texture::Layout fromLayout, Texture::Layout toLayout)=0;

    };
}
#endif //SLAG_COMMANDBUFFER_H
