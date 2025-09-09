#ifndef SLAG_VULKANBACKEND_H
#define SLAG_VULKANBACKEND_H
#include <slag/backends/Backend.h>
#include <vulkan/vulkan_core.h>

#include "VkBootstrap.h"

namespace slag
{
    namespace vulkan
    {
        struct VulkanizedFormat
        {
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkComponentMapping mapping{};
            VulkanizedFormat(VkFormat format, VkComponentSwizzle r, VkComponentSwizzle g, VkComponentSwizzle b, VkComponentSwizzle a)
            {
                this->format = format;
                this->mapping.r = r;
                this->mapping.g = g;
                this->mapping.b = b;
                this->mapping.a = a;
            }
        };

        class VulkanBackend: public Backend
        {
        public:
            static VulkanizedFormat vulkanizedFormat(Pixels::Format format);
            static VkImageUsageFlags vulkanizedUsage(Texture::UsageFlags flags);
            static VkImageType vulkanizedImageType(Texture::Type type);
            static VkImageViewType vulkanizedImageViewType(Texture::Type type, uint32_t layers);
            static VkImageAspectFlags vulkanizedAspectFlags(Pixels::AspectFlags aspectFlags);
            static VkBufferUsageFlags vulkanizedBufferUsage(Buffer::UsageFlags usageFlags);
            static VkAccessFlagBits2 vulkanizedBarrierAccessMask(BarrierAccessFlags accessFlags);
            static VkPipelineStageFlags2 vulkanizedStageMask(PipelineStageFlags stageFlags);
            static VkIndexType vulkanizedIndexType(Buffer::IndexSize indexSize);
            static VkPolygonMode vulkanizedPolygonMode(RasterizationState::DrawMode drawMode);
            static VkCullModeFlags vulkanizedCullMode(RasterizationState::CullOptions cullOptions);
            static VkFrontFace vulkanizedFrontFace(RasterizationState::FrontFacing frontFace);
            static VkBlendFactor vulkanizedBlendFactor(Operations::BlendFactor blendFactor);
            static VkBlendOp vulkanizedBlendOp(Operations::BlendOperation blendOperation);
            static VkColorComponentFlags vulkanizedColorComponentFlags(Color::ComponentFlags colorComponentFlags);
            static VkLogicOp vulkanizedLogicOp(Operations::LogicalOperation operation);
            static VkCompareOp vulkanizedCompareOp(Operations::ComparisonFunction comparisonFunction);
            static VkStencilOp vulkanizedStencilOp(Operations::StencilOperation stencilOperation);
            static VkFormat vulkanizedGraphicsType(GraphicsType graphicsType);
            static VkShaderStageFlagBits vulkanizedShaderStage(ShaderStageFlags stageFlags);
            static VkDescriptorType vulkanizedDescriptorType(Descriptor::Type descriptorType);
            static VkShaderStageFlags vulkanizedShaderFlags(ShaderStageFlags stageFlags);
            static VkFilter vulkanizedFilter(Sampler::Filter filter);
            static VkSamplerMipmapMode vulkanizedMipMapMode(Sampler::Filter mipmapFilter);
            static VkSamplerAddressMode vulkanizedAddressMode(Sampler::AddressMode addressMode);
            static VkPresentModeKHR vulkanizedPresentMode(SwapChain::PresentMode presentMode);
            static VkCompositeAlphaFlagBitsKHR vulkanizedCompositeAlphaFlags(SwapChain::AlphaCompositing composite);

            VulkanBackend(const SlagInitInfo& initInfo);
            ~VulkanBackend()override;
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
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount)override;
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)override;
#else
            virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels,Texture::SampleCount sampleCount)override;
            virtual Texture* newTexture(Pixels::Format texelFormat, TextureLayouts::Layout, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t layers, uint32_t mipLevels, Texture::SampleCount sampleCount, void* texelData, uint32_t providedDataMips, uint32_t providedDataLayers)override;
#endif
            //Buffers
            virtual Buffer* newBuffer(size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)override;
            virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)override;
            //swapchains
            virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height, SwapChain::PresentMode presentMode, uint8_t frameCount, Pixels::Format format, SwapChain::AlphaCompositing compositing, FrameResources*(* createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void (*swapchainRebuiltFunction)(SwapChain* swapChain))override;
            //samplers
            virtual Sampler* newSampler(SamplerParameters parameters)override;
            //shaders
            virtual std::vector<ShaderCode::CodeLanguage> acceptedLanuages()override;
            virtual ShaderPipeline* newShaderPipeline(ShaderCode** shaders, size_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription)override;
            virtual ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader)override;
            //descriptor pools
            virtual DescriptorPool* newDescriptorPool()override;
            virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)override;
            //descriptor groups
            virtual DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)override;
            //descriptor bundles
#ifndef SLAG_DISCREET_TEXTURE_LAYOUTS
            virtual void setDescriptorBundleSampler(DescriptorBundle& descriptor, uint32_t binding,uint32_t arrayElement, Sampler* sampler)override;
            virtual void setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)override;
            virtual void setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Texture* texture)override;
#else
            virtual void setDescriptorBundleSampler(uint32_t binding,uint32_t arrayElement, Sampler* sampler, TextureLayouts::Layout layout)override;
            virtual void setDescriptorBundleSampledTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)override;
            virtual void setStorageTexture(uint32_t binding, uint32_t arrayElement, Texture* texture, TextureLayouts::Layout layout)override;
#endif
            virtual void setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            virtual void setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            virtual void setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            virtual void setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;


            vkb::Instance vulkanInstance();
        private:
            vkb::Instance _instance{};
            VkDebugUtilsMessengerEXT _debugMessenger = nullptr;
            bool _isValid = false;
        };
    } // vulkan
} // slag

#endif //SLAG_VULKANBACKEND_H
