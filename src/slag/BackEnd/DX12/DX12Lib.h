#ifndef SLAG_DX12LIB_H
#define SLAG_DX12LIB_H
#include <directx/d3d12.h>
#include "DX12GraphicsCard.h"
#include "../BackEndLib.h"


namespace slag
{
    namespace dx
    {
        class DX12Texture;
        class DX12Lib:public lib::BackEndLib
        {
        public:
            static DX12Lib* initialize(const SlagInitDetails& details);
            static void cleanup(lib::BackEndLib* library);
            static DX12Lib* get();
            static DX12GraphicsCard* card();
            static DXGI_FORMAT format(Pixels::Format pixelFormat);
            static D3D12_RESOURCE_DIMENSION dimension(Texture::Type type);
            static D3D12_BARRIER_LAYOUT barrierLayout(Texture::Layout texLayout);
            static D3D12_RESOURCE_STATES stateLayout(Texture::Layout texLayout);
            static D3D12_TEXTURE_ADDRESS_MODE addressMode(Sampler::AddressMode mode);
            static D3D12_FILTER filter(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, bool ansitrophyEnabled);
            static D3D12_COMPARISON_FUNC comparisonFunction(Operations::ComparisonFunction compFunction);
            static uint32_t formatSize(DXGI_FORMAT format);
            static D3D12_DESCRIPTOR_RANGE_TYPE rangeType(Descriptor::DescriptorType type);

            explicit DX12Lib(DX12GraphicsCard* card);
            ~DX12Lib() override;
            BackEnd identifier()override;
            GraphicsCard* graphicsCard()override;
            //Swapchain
            Swapchain* newSwapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, Swapchain::PresentMode mode, Pixels::Format imageFormat)override;
            //Textures
            Texture* newTexture(void** texelDataArray, size_t texelDataCount, size_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, TextureUsage usage, Texture::Layout initializedLayout)override;
            Texture* newTexture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, TextureUsage usage)override;

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
            //Descriptor Pools
            DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)override;
            //Descriptor Bundles
            void setSampler(void* handle,uint32_t binding,uint32_t arrayElement, Sampler* sampler, Texture::Layout layout)override;
            void setSampledTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)override;
            void setSamplerAndTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout, Sampler* sampler)override;
            void setStorageTexture(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)override;
            void setUniformTexelBuffer(void* handle, uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            void setStorageTexelBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            void setUniformBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            void setStorageBuffer(void* handle,uint32_t binding, uint32_t arrayElement, Buffer* buffer, size_t offset, size_t length)override;
            void setInputAttachment(void* handle,uint32_t binding, uint32_t arrayElement, Texture* texture, Texture::Layout layout)override;
            void waitFor(SemaphoreValue* values, size_t count)override;

            DX12GraphicsCard* dx12GraphicsCard();
        private:
            static void mapFlags();
            DX12GraphicsCard* _graphicsCard = nullptr;

        };

    } // dx
} // slag

#endif //SLAG_DX12LIB_H
