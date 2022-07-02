#ifndef SLAGLIB_GRAPHICSCARD_H
#define SLAGLIB_GRAPHICSCARD_H

#include <cstdint>
#include <vector>

namespace slag
{

    class GraphicsCard
    {
    public:
        GraphicsCard()=delete;
        GraphicsCard(const GraphicsCard&) = delete;
        GraphicsCard& operator=(const GraphicsCard&)=delete;
        ~GraphicsCard();
        bool hasComputeCapabilities();
    private:
        GraphicsCard(void* vkbDevice);
        void* _vkDevice;
        void* _vkPhysicalDevice;
        void* _vkGraphicsQueue;
        void* _vkPresentQueue;
        uint32_t _graphicsQueueFamily;
        void* _vkComputeQueue;
        bool _hasComputeCapabilities;

        void* _vmaAllocator;

        friend void initialize(bool);
        friend class SwapChain;
        friend class RenderAttachment;
        friend class FrameBuffer;
        friend class Surface;
        friend class Renderer;
        friend class Shader;
    };
}

#endif //SLAGLIB_GRAPHICSCARD_H