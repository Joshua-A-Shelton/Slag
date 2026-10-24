// The XCB connection behind SDL's Xlib display, for --xcb.

#include <X11/Xlib-xcb.h>

void* xcbConnection(void* xlibDisplay)
{
    return XGetXCBConnection(static_cast<Display*>(xlibDisplay));
}
