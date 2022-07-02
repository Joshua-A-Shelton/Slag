#ifndef SLAGLIB_SURFACE_H
#define SLAGLIB_SURFACE_H

#include "GraphicsCard.h"
#include "Renderer.h"

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <xcb/xcb.h>
#endif

namespace slag
{
    class Surface
    {
    public:
        enum RenderingMode
        {
            singleBuffer,
            doubleBuffer,
            tripleBuffer
        };
#ifdef _WIN32
        Surface(HINSTANCE hinstance, HWND hwnd, GraphicsCard* card, bool includeDepth, RenderingMode renderingMode = RenderingMode::doubleBuffer, bool _supportAlpha = false);
#elif __linux__
        Surface(xcb_connection_t* surface, xcb_window_t& window, GraphicsCard* card, bool includeDepth, RenderingMode renderingMode = RenderingMode::doubleBuffer, bool supportAlpha = false);
#endif
        ~Surface();
        void setRenderingMode(RenderingMode mode);
        GraphicsCard* getUnderlyingGraphicsCard()const;
        RenderingMode currentRenderingMode();
        Renderer* getRenderer();
        ///Call this to explicity resize the framebuffers to match the window
        void setClearColor(float r, float g, float b, float a);
        void resizeToWindow();
        std::shared_ptr<FrameBuffer> defaultFramebuffer();
    private:
#ifdef _WIN32
        HINSTANCE _hinstance;
        HWND _hwnd;
#elif __linux__
     xcb_connection_t* _xcbConnection;
     xcb_window_t _xcbWindow;
#endif
        float clearColor[4]{1.0f,1.0f,1.0f,1.0f};
        void* _vkSurface = nullptr;
        RenderingMode _renderingStrategy;
        GraphicsCard* _graphicsCard = nullptr;
        Renderer* renderer;
        bool _supportAlpha = false;
        void init(GraphicsCard* graphicsCard, bool includeDepth, RenderingMode renderingMode, bool supportAlpha);
        friend class SwapChain;
        friend class Renderer;

    };
}

#endif //SLAGLIB_SURFACE_H