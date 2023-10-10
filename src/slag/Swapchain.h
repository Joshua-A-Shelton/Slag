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
        Swapchain* create();
    private:
        PlatformData _platformData;
        uint32_t _width = 500;
        uint32_t _height = 500;
        uint8_t _backBufferCount = 2;
        bool _vsyncEnabled = true;
        std::unordered_map<std::string,TextureResourceDescription> _textureDescriptions;
        std::unordered_set<std::string> _commandBufferNames;
        std::unordered_map<std::string, UniformBufferResourceDescription> _uniformBufferDescriptions;
    };
}
#endif //SLAG_SWAPCHAIN_H