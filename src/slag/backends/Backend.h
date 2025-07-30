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
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)=0;
        virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)=0;
#else
        virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)=0;
        virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)=0;
#endif
        //Buffers
        virtual Buffer* newBuffer(size_t size, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)=0;
        virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)=0;
        //swapchains
        virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))=0;
        //samplers
        virtual Sampler* newSampler(SamplerParameters parameters)=0;
        //shaders
        virtual std::vector<ShaderCode::CodeLanguage> acceptedLanuages()=0;
        virtual ShaderPipeline* newShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)=0;
        virtual ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader)=0;
        //descriptor pools
        virtual DescriptorPool* newDescriptorPool()=0;
        virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)=0;
        //descriptor groups
        virtual DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)=0;
        //descriptor bundles
#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
        virtual void setDescriptorBundleSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, TextureLayouts::Layout layout);
        virtual void setDescriptorBundleSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout);
        virtual void setDescriptorBundleSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout, Sampler* sampler);
        virtual void setDescriptorBundleStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout);
        virutal void setDescriptorBundleInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout);
#else
        virtual void setDescriptorBundleSampler(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Sampler* sampler)=0;
        virtual void setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)=0;
        virtual void setDescriptorBundleTextureAndSampler(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture, Sampler* sampler)=0;
        virtual void setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)=0;
        virtual void setDescriptorBundleInputAttachment(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)=0;
#endif
        virtual void setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
        virtual void setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
        virtual void setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
        virtual void setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;

    };
}
#endif //SLAG_BACKEND_H