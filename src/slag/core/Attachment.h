#ifndef SLAG_ATTACHMENT_H
#define SLAG_ATTACHMENT_H
#include "Clear.h"

namespace slag
{
    class Texture;
    class GraphicsCard;
    ///Descriptor for a frame buffer target. It shouldn't have to exist, it's supposedly just cpu accessible memory, but the current API's don't expose a way to manage it ourselves
    class FrameBufferView
    {
    public:
        virtual ~FrameBufferView()=default;
        ///The texture the view looks into
        virtual Texture* texture()=0;
        ///The mip level the view exposes
        virtual uint32_t mip()=0;
        ///The first array layer to draw into
        virtual uint32_t baseLayer()=0;
        ///The total count of layers to draw into
        virtual uint32_t layerCount()=0;
        ///Graphics card the descriptor data is valid for
        virtual GraphicsCard* graphicsCard()=0;
    };
    ///Defines an attachment (color or depth target) to be used for CommandBuffer::beginRendering
    struct Attachment
    {
        ///The frame buffer view to target
        FrameBufferView* bufferView = nullptr;
        ///clears the texture on beginRending automatically if true
        bool autoClear = false;
        //the clear value to clear the texture to if autoClear is true
        ClearValue clearValue={0.0f,0.0f,0.0f,1.0f};
    };

}
#endif //SLAG_ATTACHMENT_H