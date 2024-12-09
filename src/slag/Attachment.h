#ifndef SLAG_ATTACHMENT_H
#define SLAG_ATTACHMENT_H
#include "Texture.h"
#include "Clear.h"
namespace slag
{

    ///Defines an attachment (color or depth target) to be used for CommandBuffer::beginRendering
    struct Attachment
    {
        ///The texture to target
        Texture* texture = nullptr;
        ///The layout the texture will be in at the start of the beginRendering call (RENDER_TARGET/GENERAL for color attachments, DEPTH_TARGET/DEPTH_TARGET_READ_ONLY/GENERAL for depth attachments)
        Texture::Layout layout = Texture::UNDEFINED;
        ///clears the texture on beginRending automatically if true
        bool clearOnLoad = false;
        //the clear value to clear the texture to if clearOnLoad is true
        ClearValue clear={0.0f,0.0f,0.0f,0.0f};
    };

} // slag

#endif //SLAG_ATTACHMENT_H
