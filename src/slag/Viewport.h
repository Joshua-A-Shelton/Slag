#ifndef CRUCIBLEEDITOR_VIEWPORT_H
#define CRUCIBLEEDITOR_VIEWPORT_H

#include "Rectangle.h"
namespace slag
{

    struct Viewport
    {
        Rectangle view;
        float minDepth;
        float maxDepth;
    };

} // slag

#endif //CRUCIBLEEDITOR_VIEWPORT_H
