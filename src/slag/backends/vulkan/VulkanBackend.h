#ifndef SLAG_VULKANBACKEND_H
#define SLAG_VULKANBACKEND_H
#include <slag/backends/Backend.h>
namespace slag
{
    namespace vulkan
    {
        class VulkanBackend: public Backend
        {
        public:
            VulkanBackend(const SlagInitInfo& initInfo);
            ~VulkanBackend()override;

            virtual std::vector<std::unique_ptr<GraphicsCard>> getGraphicsCards()override;
            virtual GraphicsBackend backendAPI()override;

            //command buffers
            virtual CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands)override;
            virtual CommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer)override;
            //semaphores
            virtual Semaphore* newSemaphore(uint64_t initialValue)override;
            virtual void waitFor(SemaphoreValue* values, size_t count)override;
            //textures
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels)override;
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)override;
            //swapchains
            virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))override;

        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBACKEND_H
