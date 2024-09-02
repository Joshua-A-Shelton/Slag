#ifndef SLAG_PLATFORMDATA_H
#define SLAG_PLATFORMDATA_H

namespace slag
{

    struct PlatformData
    {
        void* nativeWindowHandle = nullptr;
        void* nativeDisplayType = nullptr;
    };

} // slag

#endif //SLAG_PLATFORMDATA_H
