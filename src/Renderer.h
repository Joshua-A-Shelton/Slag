#ifndef SLAGLIB_RENDERER_H
#define SLAGLIB_RENDERER_H
#include <vector>
#include "FrameBuffer.h"
#include "Shader.h"
namespace slag
{
    class Surface;

    class Renderer
    {
    public:
        Renderer()=delete;
        Renderer(const Renderer&)=delete;
        Renderer& operator=(const Renderer&)=delete;
        ~Renderer();
        void beginFrame();
        void bindDefaultFrameBuffer();
        void bindFrameBuffer(std::shared_ptr<FrameBuffer> frameBuffer);
        void unBindCurrentFrameBuffer();
        void bindShader(Shader& shader);
        void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void endFrame();
    private:
        Renderer(Surface* surface, bool includeDepth);
        void setDefaultClearColor(float* color);
        Surface* _surface;
        void* _vkDevice;
        void* _swapchain;
        void* _currentSwapchainCommandBuffer;
        bool _hasStartedFrame = false;
        void recreateSwapChain();
        friend class Surface;
    };
}

#endif //SLAGLIB_RENDERER_H