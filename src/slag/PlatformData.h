#ifndef SLAG_PLATFORMDATA_H
#define SLAG_PLATFORMDATA_H

namespace slag
{
    ///Platform specific data required for creating a swapchain
    struct PlatformData
    {
        ///A window's HWND (Windows) or native window handle (Unix)
        void* nativeWindowHandle = nullptr;
        ///A window's HINSTANCE (Windows) or display(Unix)
        void* nativeDisplayType = nullptr;
    };

} // slag

#endif //SLAG_PLATFORMDATA_H
