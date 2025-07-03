#ifndef SLAG_ATTACHMENT_H
#define SLAG_ATTACHMENT_H
#include "Clear.h"

namespace slag
{
    class Texture;
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
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
#else
    ///Defines an attachment (color or depth target) to be used for CommandBuffer::beginRendering
    struct AttachmentDiscreet
    {
        ///The texture to target
        Texture* texture = nullptr;
        ///The layout the texture will be in at the start of the beginRendering call (RENDER_TARGET/GENERAL for color attachments, DEPTH_TARGET/DEPTH_TARGET_READ_ONLY/GENERAL for depth attachments)
        TextureLayouts::Layout layout = TextureLayouts::UNDEFINED;
        ///clears the texture on beginRending automatically if true
        bool autoClear = false;
        //the clear value to clear the texture to if autoClear is true
        ClearValue clearValue={0.0f,0.0f,0.0f,1.0f};
    };
#endif


}
#endif //SLAG_ATTACHMENT_H
