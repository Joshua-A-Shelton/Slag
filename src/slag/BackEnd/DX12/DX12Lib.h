#ifndef SLAG_DX12LIB_H
#define SLAG_DX12LIB_H
#include <directx/d3d12.h>
#include "DX12GraphicsCard.h"
#include "../BackEndLib.h"


namespace slag
{
    namespace dx
    {

        class DX12Lib:public lib::BackEndLib
        {
        public:
            static DX12Lib* initialize();
            static void cleanup(lib::BackEndLib* library);
            static DX12Lib* get();
            static DX12GraphicsCard* card();
            static DXGI_FORMAT format(Pixels::Format pixelFormat);
            static D3D12_BARRIER_LAYOUT barrierLayout(Texture::Layout texLayout);
            static D3D12_RESOURCE_STATES stateLayout(Texture::Layout texLayout);
            static D3D12_TEXTURE_ADDRESS_MODE addressMode(Sampler::AddressMode mode);
            static D3D12_FILTER filter(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, bool ansitrophyEnabled);
            static D3D12_COMPARISON_FUNC comparisonFunction(Operations::ComparisonFunction compFunction);
            explicit DX12Lib(DX12GraphicsCard* card);
            ~DX12Lib() override;
            BackEnd identifier()override;
            GraphicsCard* graphicsCard()override;
            //Swapchain
            Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)override;
            //Textures
            Texture* newTexture(void* data, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage, Texture::Layout initializedLayout)override;
            Texture* newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)override;
            //CommandBuffers
            CommandBuffer* newCommandBuffer(GpuQueue::QueueType acceptsCommands)override;
            //Buffers
            Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
            Buffer* newBuffer(size_t  bufferSize, Buffer::Accessibility accessibility, Buffer::Usage usage)override;
            //Semaphores
            Semaphore* newSemaphore(uint64_t startingValue)override;
            //Samplers
            Sampler* newSampler(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, Sampler::AddressMode u, Sampler::AddressMode v, Sampler::AddressMode w, float mipLODBias, bool enableAnisotrophy, uint8_t maxAnisotrophy,Operations::ComparisonFunction comparisonFunction, Color borderColor, float minLOD, float maxLOD)override;
            //Shaders
            DescriptorGroup* newDescriptorGroup(Descriptor* descriptors, size_t descriptorCount)override;
            Shader* newShader(ShaderModule* modules, size_t moduleCount, DescriptorGroup** descriptorGroups, size_t descriptorGroupCount, ShaderProperties& properties, VertexDescription* vertexDescription, FrameBufferDescription& frameBufferDescription)override;

            void waitFor(SemaphoreValue* values, size_t count)override;

            DX12GraphicsCard* dx12GraphicsCard();
        private:
            static void mapFlags();
            DX12GraphicsCard* _graphicsCard = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12LIB_H
