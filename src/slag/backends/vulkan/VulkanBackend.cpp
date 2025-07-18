#include "VulkanBackend.h"

namespace slag
{
    namespace vulkan
    {

        VulkanBackend::VulkanBackend(const SlagInitInfo& initInfo)
        {
            if (initInfo.slagDebugHandler)
            {

            }
        }

        VulkanBackend::~VulkanBackend()
        {
            throw std::runtime_error("Not implemented");
        }

        std::vector<std::unique_ptr<GraphicsCard>> VulkanBackend::getGraphicsCards()
        {
            throw std::runtime_error("Not implemented");
        }

        GraphicsBackend VulkanBackend::backendAPI()
        {
            throw std::runtime_error("Not implemented");
        }

        CommandBuffer* VulkanBackend::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
        {
            throw std::runtime_error("Not implemented");
        }

        CommandBuffer* VulkanBackend::newSubCommandBuffer(CommandBuffer* parentBuffer)
        {
            throw std::runtime_error("Not implemented");
        }

        Semaphore* VulkanBackend::newSemaphore(uint64_t initialValue)
        {
            throw std::runtime_error("Not implemented");
        }

        void VulkanBackend::waitFor(SemaphoreValue* values, size_t count)
        {
            throw std::runtime_error("Not implemented");
        }

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount)
        {
            throw std::runtime_error("Not implemented");
        }


        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            throw std::runtime_error("Not implemented");
        }
#else
        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)
        {
            throw std::runtime_error("Not implemented");
        }
        Texture* VulkanBackend::newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            throw std::runtime_error("Not implemented");
        }


#endif

        Buffer* VulkanBackend::newBuffer(size_t dataSize, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            throw std::runtime_error("Not implemented");
        }

        Buffer* VulkanBackend::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            throw std::runtime_error("Not implemented");
        }

        SwapChain* VulkanBackend::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format,
            FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain))
        {
            throw std::runtime_error("Not implemented");
        }

        Sampler* VulkanBackend::newSampler(SamplerParameters parameters)
        {
            throw std::runtime_error("Not implemented");
        }

        std::vector<ShaderCode::CodeLanguage> VulkanBackend::acceptedLanuages()
        {
            throw std::runtime_error("Not implemented");
        }

        ShaderPipeline* VulkanBackend::newShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription,
            FrameBufferDescription& framebufferDescription)
        {
            throw std::runtime_error("Not implemented");
        }

        ShaderPipeline* VulkanBackend::newShaderPipeline(const ShaderCode& computeShader)
        {
            throw std::runtime_error("Not implemented");
        }

        DescriptorPool* VulkanBackend::newDescriptorPool()
        {
            throw std::runtime_error("Not implemented");
        }

        DescriptorPool* VulkanBackend::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            throw std::runtime_error("Not implemented");
        }

        DescriptorGroup* VulkanBackend::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            throw std::runtime_error("Not implemented");
        }

#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
        void VulkanBackend::setDescriptorBundleSampler(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Sampler* sampler)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleTextureAndSampler(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture, Sampler* sampler)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleInputAttachment(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("Not implemented");
        }
#else
        void VulkanBackend::setDescriptorBundleSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, TextureLayouts::Layout layout)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setSamplerAndTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout, Sampler* sampler)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setInputAttachment(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)
        {
            throw std::runtime_error("Not implemented");
        }
#endif
        void VulkanBackend::setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("Not implemented");
        }
        void VulkanBackend::setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("Not implemented");
        }
    } // vulkan
} // slag
