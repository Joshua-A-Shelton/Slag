#ifndef SLAG_BACKEND_H
#define SLAG_BACKEND_H
#include <memory>
#include <vector>
#include <slag/Slag.h>

#include "slag/core/DescriptorPool.h"

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
        virtual void postGraphicsCardChosenSetup()=0;
        virtual void preGraphicsCardDestroyCleanup()=0;

        static Backend* current(){return _current;}

        virtual bool valid()=0;

        virtual std::vector<std::unique_ptr<GraphicsCard>> getGraphicsCards()=0;

        virtual GraphicsBackend backendAPI()=0;
        //command buffers
        virtual CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands)=0;
        virtual CommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer)=0;
        //semaphores
        virtual Semaphore* newSemaphore(uint64_t initialValue)=0;
        virtual void waitFor(SemaphoreValue* values, size_t count)=0;
        //textures
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)=0;
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)=0;
        //Buffers
        virtual Buffer* newBuffer(size_t size, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)=0;
        virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)=0;
        //BufferViews
        virtual BufferView* newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)=0;
        //swapchains
        virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format, SwapChain::AlphaCompositing compositing, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain), void (*swapchainRebuiltFunction)(SwapChain* swapChain))=0;
        //samplers
        virtual Sampler* newSampler(SamplerParameters parameters)=0;
        //shaders
        virtual std::vector<ShaderCode::CodeLanguage> acceptedLanuages()=0;
        virtual ShaderPipeline* newShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)=0;
        virtual ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader,std::string(*rename)(const DescriptorRenameParameters&,void*), void*)=0;
        //descriptor pools
        virtual DescriptorPool* newDescriptorPool()=0;
        virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)=0;
        //descriptor bundles
        virtual void setDescriptorBundleSampler(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, Sampler* sampler)=0;
        virtual void setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Texture* texture)=0;
        virtual void setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Texture* texture)=0;
        virtual void setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)=0;
        virtual void setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)=0;
        virtual void setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        virtual void setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)=0;
        //Pixel Properties
        virtual PixelFormatProperties pixelFormatProperties(Pixels::Format format)=0;

    };
}
#endif //SLAG_BACKEND_H