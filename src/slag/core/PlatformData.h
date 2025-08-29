#ifndef SLAG_PLATFORMDATA_H
#define SLAG_PLATFORMDATA_H

#ifdef SLAG_WIN32_BACKEND
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#define SLAG_WINDOWS_HWND HWND
#define SLAG_WINDOWS_HINSTANCE HINSTANCE
#else
#define SLAG_WINDOWS_HWND void*
#define SLAG_WINDOWS_HINSTANCE void*
#endif

#ifdef SLAG_X11_BACKEND
#include <X11/Xlib.h>
#undef None
#undef Bool
#define SLAG_X11_WINDOW Window
#define SLAG_X11_DISPLAY Display*
#else
#define SLAG_X11_WINDOW void*
#define SLAG_X11_DISPLAY void*
#endif

#ifdef SLAG_WAYLAND_BACKEND
#include <wayland-client.h>
#define SLAG_WAYLAND_SURFACE wl_surface*
#define SLAG_WAYLAND_DISPLAY wl_display*
#else
#define SLAG_WAYLAND_SURFACE void*
#define SLAG_WAYLAND_DISPLAY void*
#endif

namespace slag
{
    struct Win32PlatformData
    {
        ///Window Handle
        SLAG_WINDOWS_HWND hwnd{};
        ///Program instance the window is tied to
        SLAG_WINDOWS_HINSTANCE hinstance{};
    };
    struct X11PlatformData
    {
        SLAG_X11_WINDOW window{};
        SLAG_X11_DISPLAY display{};
    };
    struct WaylandPlatformData
    {
        SLAG_WAYLAND_SURFACE surface{};
        SLAG_WAYLAND_DISPLAY display{};
    };
    struct CustomPlatformData
    {
        void* data=nullptr;
    };

    union PlatformDetails
    {
        Win32PlatformData win32;
        X11PlatformData x11;
        WaylandPlatformData wayland;
        CustomPlatformData custom;
    };
    enum class Platform
    {
        WIN_32,
        X11,
        WAYLAND,
        CUSTOM
    };
    ///Platform specific data required for creating a swapchain
    struct PlatformData
    {
        Platform platform = Platform::WIN_32;
        PlatformDetails details{};
    };

} // slag

#endif //SLAG_PLATFORMDATA_H
