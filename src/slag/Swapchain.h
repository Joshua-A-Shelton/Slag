#ifndef SLAG_SWAPCHAIN_H
#define SLAG_SWAPCHAIN_H
#include "Frame.h"
#include "PlatformData.h"
#include "ResourceDescriptions.h"
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace slag
{
    class Swapchain
    {
    public:
        Swapchain();
        virtual ~Swapchain();
        virtual Frame* currentFrame()=0;
        virtual Frame* next()=0;
        virtual size_t backBufferCount()=0;
        virtual void backBufferCount(size_t count)=0;
        virtual bool vsyncEnabled()=0;
        virtual void vsyncEnabled(bool enabled)=0;
        virtual uint32_t width()=0;
        virtual uint32_t height()=0;
        virtual void resize(uint32_t width, uint32_t height)=0;
        virtual Pixels::PixelFormat imageFormat()=0;
        virtual void setResource(const char* name, TextureResourceDescription description)=0;
        virtual void setResource(const char* name, VertexBufferResourceDescription description)=0;
        virtual void setResource(const char* name, IndexBufferResourceDescription description)=0;
        virtual void setResources(const char** textureNames, TextureResourceDescription* textureDescriptions, size_t textureCount,
                                     const char** vertexBufferNames, VertexBufferResourceDescription* vertexBufferDescriptions, size_t vertexBufferCount,
                                     const char** indexBufferNames, IndexBufferResourceDescription* indexBufferDescriptions, size_t indexBufferCount)=0;
    };

    class SwapchainBuilder
    {
    public:
        SwapchainBuilder(PlatformData platformData);
        SwapchainBuilder& setWidth(uint32_t width);
        SwapchainBuilder& setHeight(uint32_t height);
        SwapchainBuilder& setDesiredBackBuffers(uint8_t count);
        SwapchainBuilder& setVSyncEnabled(bool enabled);
        SwapchainBuilder& addTextureResource(std::string name, TextureResourceDescription description);
        SwapchainBuilder& addCommandBufferResource(std::string name);
        SwapchainBuilder& addUniformBufferResource(std::string name, UniformBufferResourceDescription description);
        SwapchainBuilder& addVertexBufferResource(std::string name, VertexBufferResourceDescription description);
        SwapchainBuilder& addIndexBufferResource(std::string name, IndexBufferResourceDescription description);
        SwapchainBuilder& setDesiredPixelFormat(Pixels::PixelFormat  format);
        SwapchainBuilder& setDrawOnMinimized(bool draw);
        Swapchain* create();
    private:
        PlatformData _platformData;
        uint32_t _width = 500;
        uint32_t _height = 500;
        uint8_t _backBufferCount = 2;
        Pixels::PixelFormat _defaultFormat = Pixels::PixelFormat::B8G8R8A8_UNORM;
        bool _vsyncEnabled = true;
        bool _drawOnMinimized = false;
        std::unordered_map<std::string,TextureResourceDescription> _textureDescriptions;
        std::unordered_set<std::string> _commandBufferNames;
        std::unordered_map<std::string, UniformBufferResourceDescription> _uniformBufferDescriptions;
        std::unordered_map<std::string, VertexBufferResourceDescription> _vertexBufferDescriptions;
        std::unordered_map<std::string, IndexBufferResourceDescription> _indexBufferDescriptions;
    };
}
#endif //SLAG_SWAPCHAIN_H