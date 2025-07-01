#ifndef SLAG_BACKEND_H
#define SLAG_BACKEND_H
#include <memory>
#include <vector>
#include <slag/Slag.h>

namespace slag
{
    class Backend
    {
    private:
        static inline Backend* _current=nullptr;
    public:
        friend SlagInitializationResult slag::initialize(const SlagInitInfo& initInfo);
        friend void slag::cleanup();

        virtual ~Backend()=default;

        static Backend* current(){return _current;}

        virtual std::vector<std::unique_ptr<GraphicsCard>> getGraphicsCards()=0;

        virtual GraphicsBackend backendAPI()=0;
        //command buffers
        virtual CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands)=0;
        virtual CommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer)=0;
        //semaphores
        virtual Semaphore* newSemaphore(uint64_t initialValue)=0;
        virtual void waitFor(SemaphoreValue* values, size_t count)=0;
        //textures
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels)=0;
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)=0;
        //swapchains
        virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))=0;

    };
}
#endif //SLAG_BACKEND_H