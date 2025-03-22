#ifndef SLAG_BACKENDLIB_H
#define SLAG_BACKENDLIB_H
#include "../SlagLib.h"
#include <spirv_reflect.h>

namespace slag
{
    namespace lib
    {
        class BackEndLib
        {
        public:
            static BackEndLib* get();
            static void set(BackEndLib* library);
            virtual ~BackEndLib()=default;
            virtual BackEnd identifier()=0;
            virtual GraphicsCard* graphicsCard()=0;
            //Swapchain
            virtual Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat,FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain))=0;
            //Textures
            virtual Texture* newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)=0;
            virtual Texture* newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage)=0;
            //CommandBuffers
            virtual CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands)=0;
            //Buffers
            virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)=0;
            virtual Buffer* newBuffer(size_t  bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)=0;
            //Semaphores
            virtual Semaphore* newSemaphore(uint64_t startingValue)=0;
            virtual void waitFor(SemaphoreValue* values, size_t count)=0;
            //Samplers
            virtual Sampler* newSampler(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, Sampler::AddressMode u, Sampler::AddressMode v, Sampler::AddressMode w, float mipLODBias, bool enableAnisotrophy, uint8_t maxAnisotrophy,Operations::ComparisonFunction comparisonFunction, Color borderColor, float minLOD, float maxLOD)=0;
            //Shaders
            virtual DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)=0;
            virtual ShaderPipeline* newShaderPipeline(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)=0;
            virtual ShaderPipeline* newShaderPipeline(ShaderModule** modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)=0;
            //Descriptor Pools
            virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)=0;
            //Descriptor Bundles
            virtual void setSampler(void* handle,uint32_t binding,uint32_t arrayElement, Sampler* sampler, Texture::Layout layout)=0;
            virtual void setSampledTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)=0;
            virtual void setSamplerAndTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout, Sampler* sampler)=0;
            virtual void setStorageTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)=0;
            virtual void setUniformTexelBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
            virtual void setStorageTexelBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
            virtual void setUniformBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
            virtual void setStorageBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)=0;
            virtual void setInputAttachment(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)=0;

            static Descriptor::DescriptorType descriptorTypeFromSPV(SpvReflectDescriptorType type);
            static GraphicsTypes::GraphicsType graphicsTypeFromSPV(SpvReflectFormat format);
            static GraphicsTypes::GraphicsType graphicsTypeFromSPV(SpvReflectTypeDescription* typeDescription);
            static UniformBufferDescriptorLayout uniformBufferDescriptorLayoutFromSPV(SpvReflectBlockVariable* block);
        };

    }
}
#endif //SLAG_BACKENDLIB_H
