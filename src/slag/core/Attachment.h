#ifndef SLAG_ATTACHMENT_H
#define SLAG_ATTACHMENT_H
#include "Clear.h"

namespace slag
{
    class Texture;
    ///Defines an attachment (color or depth target) to be used for CommandBuffer::beginRendering
    struct Attachment
    {
        ///The texture to target
        Texture* texture = nullptr;
        ///clears the texture on beginRending automatically if true
        bool autoClear = false;
        //the clear value to clear the texture to if autoClear is true
        ClearValue clearValue={0.0f,0.0f,0.0f,1.0f};
    };

}
#endif //SLAG_ATTACHMENT_H
