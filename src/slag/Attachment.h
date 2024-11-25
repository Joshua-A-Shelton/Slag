#ifndef SLAG_ATTACHMENT_H
#define SLAG_ATTACHMENT_H
#include "Texture.h"
#include "Clear.h"
namespace slag
{

    struct Attachment
    {
        Texture* texture = nullptr;
        bool clearOnLoad = false;
        ClearValue clear={0.0f,0.0f,0.0f,0.0f};
    };

} // slag

#endif //SLAG_ATTACHMENT_H
