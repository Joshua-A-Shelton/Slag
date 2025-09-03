#ifndef SLAG_DX12BACKEND_H
#define SLAG_DX12BACKEND_H
#include <slag/Slag.h>
#include "../Backend.h"

namespace slag
{
    namespace dx12
    {
        class DX12Backend : public Backend
        {
            virtual ~DX12Backend()override;

            virtual bool valid()override;

            virtual std::vector<std::unique_ptr<GraphicsCard>> getGraphicsCards()override;

            virtual GraphicsBackend backendAPI()override;
            //command buffers
            virtual CommandBuffer* newCommandBuffer(GPUQueue::QueueType acceptsCommands)override;
            virtual CommandBuffer* newSubCommandBuffer(CommandBuffer* parentBuffer)override;
            //semaphores
            virtual Semaphore* newSemaphore(uint64_t initialValue)override;
            virtual void waitFor(SemaphoreValue* values, size_t count)override;
            //textures
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)override;
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips,uint32_t providedDataLayers)override;
#else
        virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)override;
        virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)override;
#endif
            //Buffers
            virtual Buffer* newBuffer(size_t size, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)override;
            virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)override;
            //swapchains
            virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height,SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount,Pixels::Format format, SwapChain::AlphaCompositing compositing,FrameResources*(*createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void (*swapchainRebuiltFunction)(SwapChain* swapChain))override;
            //samplers
            virtual Sampler* newSampler(SamplerParameters parameters)override;
            //shaders
            virtual std::vector<ShaderCode::CodeLanguage> acceptedLanuages()override;
            virtual ShaderPipeline* newShaderPipeline(ShaderCode** shaders, size_t shaderCount,ShaderProperties& properties,VertexDescription& vertexDescription,FrameBufferDescription& framebufferDescription)override;
            virtual ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader)override;
            //descriptor pools
            virtual DescriptorPool* newDescriptorPool();
            virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)override;
            //descriptor groups
            virtual DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)override;
            //descriptor bundles
#ifdef SLAG_DISCREET_TEXTURE_LAYOUTS
        virtual void setDescriptorBundleSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, TextureLayouts::Layout layout)override;
        virtual void setDescriptorBundleSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)override;
        virtual void setDescriptorBundleSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout, Sampler* sampler)override;
        virtual void setDescriptorBundleStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)override;
        virutal void setDescriptorBundleInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)override;
#else
            virtual void setDescriptorBundleSampler(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Sampler* sampler)override;
            virtual void setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Texture* texture)override;
            virtual void setDescriptorBundleTextureAndSampler(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Texture* texture,Sampler* sampler)override;
            virtual void setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Texture* texture)override;
            virtual void setDescriptorBundleInputAttachment(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Texture* texture)override;
#endif
            virtual void setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)override;
            virtual void setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)override;
            virtual void setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)override;
            virtual void setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Buffer* buffer, size_t offset,size_t length)override;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BACKEND_H
