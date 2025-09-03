#include "DX12Backend.h"

namespace slag
{
    namespace dx12
    {
        DX12Backend::~DX12Backend()
        {
        }

        bool DX12Backend::valid()
        {
            return false;
        }

        std::vector<std::unique_ptr<GraphicsCard>> DX12Backend::getGraphicsCards()
        {
            throw std::runtime_error("DX12Backend::getGraphicsCards() not implemented");
        }

        GraphicsBackend DX12Backend::backendAPI()
        {
            return GraphicsBackend::DX12_GRAPHICS_BACKEND;
        }

        CommandBuffer* DX12Backend::newCommandBuffer(GPUQueue::QueueType acceptsCommands)
        {
            throw std::runtime_error("DX12Backend::newCommandBuffer() not implemented");
        }

        CommandBuffer* DX12Backend::newSubCommandBuffer(CommandBuffer* parentBuffer)
        {
            throw std::runtime_error("DX12Backend::newSubCommandBuffer() not implemented");
        }

        Semaphore* DX12Backend::newSemaphore(uint64_t initialValue)
        {
            throw std::runtime_error("DX12Backend::newSemaphore() not implemented");
        }

        void DX12Backend::waitFor(SemaphoreValue* values, size_t count)
        {
            throw std::runtime_error("DX12Backend::waitFor() not implemented");
        }

        Texture* DX12Backend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount)
        {
            throw std::runtime_error("DX12Backend::newTexture() not implemented");
        }

        Texture* DX12Backend::newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags,uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount,void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)
        {
            throw std::runtime_error("DX12Backend::newTexture() not implemented");
        }

        Buffer* DX12Backend::newBuffer(size_t size, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)
        {
            throw std::runtime_error("DX12Backend::newBuffer() not implemented");
        }

        Buffer* DX12Backend::newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)
        {
            throw std::runtime_error("DX12Backend::newBuffer() not implemented");
        }

        SwapChain* DX12Backend::newSwapChain(PlatformData platformData, uint32_t width, uint32_t height,SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount, Pixels::Format format,SwapChain::AlphaCompositing compositing,FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void(* swapchainRebuiltFunction)(SwapChain* swapChain))
        {
            throw std::runtime_error("DX12Backend::newSwapChain() not implemented");
        }

        Sampler* DX12Backend::newSampler(SamplerParameters parameters)
        {
            throw std::runtime_error("DX12Backend::newSampler() not implemented");
        }

        std::vector<ShaderCode::CodeLanguage> DX12Backend::acceptedLanuages()
        {
            throw std::runtime_error("DX12Backend::acceptedLanuages() not implemented");
        }

        ShaderPipeline* DX12Backend::newShaderPipeline(ShaderCode** shaders, size_t shaderCount,ShaderProperties& properties, VertexDescription& vertexDescription,FrameBufferDescription& framebufferDescription)
        {
            throw std::runtime_error("DX12Backend::newShaderPipeline() not implemented");
        }

        ShaderPipeline* DX12Backend::newShaderPipeline(const ShaderCode& computeShader)
        {
            throw std::runtime_error("DX12Backend::newShaderPipeline() not implemented");
        }

        DescriptorPool* DX12Backend::newDescriptorPool()
        {
            throw std::runtime_error("DX12Backend::newDescriptorPool() not implemented");
        }

        DescriptorPool* DX12Backend::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
        {
            throw std::runtime_error("DX12Backend::newDescriptorPool() not implemented");
        }

        DescriptorGroup* DX12Backend::newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)
        {
            throw std::runtime_error("DX12Backend::newDescriptorGroup() not implemented");
        }

        void DX12Backend::setDescriptorBundleSampler(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Sampler* sampler)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleSampler() not implemented");
        }

        void DX12Backend::setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleSampledTexture() not implemented");
        }

        void DX12Backend::setDescriptorBundleTextureAndSampler(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Texture* texture, Sampler* sampler)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleTextureAndSampler() not implemented");
        }

        void DX12Backend::setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageTexture() not implemented");
        }

        void DX12Backend::setDescriptorBundleInputAttachment(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Texture* texture)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleInputAttachment() not implemented");
        }

        void DX12Backend::setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleUniformTexelBuffer() not implemented");
        }

        void DX12Backend::setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageTexelBuffer() not implemented");
        }

        void DX12Backend::setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleUniformBuffer() not implemented");
        }

        void DX12Backend::setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, uint32_t binding,
            uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)
        {
            throw std::runtime_error("DX12Backend::setDescriptorBundleStorageBuffer() not implemented");
        }
    } // dx12
} // slag
