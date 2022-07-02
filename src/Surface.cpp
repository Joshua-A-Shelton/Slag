#include "Surface.h"
#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#elif __linux__
#define VK_USE_PLATFORM_XCB_KHR
#endif
#include <VkBootstrap.h>
#include "BackEnd/VulkanBackEnd.h"
#include <stdexcept>
#include "BackEnd/SwapChain.h"
#include "BackEnd/VulkanBackEnd.h"

namespace slag
{
#ifdef _WIN32
    Surface::Surface(HINSTANCE hinstance, HWND hwnd, GraphicsCard* graphicsCard, uint32_t width, uint32_t height, bool includeDepth, RenderingMode renderingMode, bool supportAlpha)
    {
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.hinstance = windowInstance;
        surfaceCreateInfo.hwnd = window;

        if(vkCreateWin32SurfaceKHR(slag::_vulkanInstance, &surfaceCreateInfo, nullptr,&surface)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create renderable surface");
        }
        _vkSurface = surface;
        init(graphicsCard, width, height, includeDepth, renderingMode, supportAlpha);

    }

#elif __linux__

    Surface::Surface(xcb_connection_t* connection, xcb_window_t& window, GraphicsCard* graphicsCard, bool includeDepth, RenderingMode renderingMode, bool supportAlpha)
    {

        _xcbConnection = connection;
        _xcbWindow = window;

        VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.connection = connection;
        surfaceCreateInfo.window = window;

        VkSurfaceKHR surface;

        if(vkCreateXcbSurfaceKHR(slag::_vulkanInstance(), &surfaceCreateInfo, nullptr,&surface)!= VK_SUCCESS)
        {
            throw std::runtime_error("Unable to create renderable surface");
        }
        _vkSurface = surface;
        init(graphicsCard,includeDepth,renderingMode,supportAlpha);

    }
#endif

    void Surface::init(GraphicsCard* graphicsCard, bool includeDepth, RenderingMode renderingMode, bool supportAlpha)
    {
        _graphicsCard = graphicsCard;
        _renderingStrategy = renderingMode;
        _supportAlpha = supportAlpha;

        renderer = new Renderer(this,includeDepth);
    }

    void Surface::setRenderingMode(Surface::RenderingMode mode)
    {
        _renderingStrategy = mode;
        renderer->recreateSwapChain();
    }


    GraphicsCard *Surface::getUnderlyingGraphicsCard() const
    {
        return _graphicsCard;
    }

    Surface::RenderingMode Surface::currentRenderingMode()
    {
        return _renderingStrategy;
    }

    Renderer *Surface::getRenderer()
    {
        return renderer;
    }

    Surface::~Surface()
    {
        delete renderer;
        vkDestroySurfaceKHR(slag::_vulkanInstance(),static_cast<VkSurfaceKHR>(_vkSurface), nullptr);
    }


    void Surface::setClearColor(float r, float g, float b, float a)
    {
        clearColor[0] = r;
        clearColor[1] = g;
        clearColor[2] = b;
        clearColor[3] = a;
        renderer->setDefaultClearColor(clearColor);
    }

    void Surface::resizeToWindow()
    {
        renderer->recreateSwapChain();
    }

    std::shared_ptr<FrameBuffer> Surface::defaultFramebuffer()
    {
        return static_cast<SwapChain*>(renderer->_swapchain)->getCurrentFrameBuffer();
    }


}