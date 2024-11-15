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
            virtual Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)=0;
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
            virtual Shader* newShader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)=0;

            static Descriptor::DescriptorType descriptorTypeFromSPV(SpvReflectDescriptorType type);
            static GraphicsTypes::GraphicsType graphicsTypeFromSPV(SpvReflectFormat format);
        };

    }
}
#endif //SLAG_BACKENDLIB_H
