#ifndef SLAG_DX12BACKEND_H
#define SLAG_DX12BACKEND_H
#include <slag/Slag.h>
#include "../Backend.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace slag
{
    namespace dx12
    {
        class DX12Backend : public Backend
        {
        public:

            static DXGI_FORMAT dx12Format(Pixels::Format format);
            static D3D12_RESOURCE_DIMENSION dx12Dimension(Texture::Type type);
            static D3D12_RESOURCE_FLAGS dx12UsageFlags(Texture::UsageFlags usage);
            static D3D12_BLEND dx12blendFactor(Operations::BlendFactor blendFactor);
            static D3D12_BLEND_OP dx12BlendOp(Operations::BlendOperation blendOperation);
            static D3D12_LOGIC_OP  dx12LogicOp(Operations::LogicalOperation operation);
            static D3D12_FILL_MODE dx12FillMode(RasterizationState::DrawMode drawMode);
            static D3D12_CULL_MODE dx12CullMode(RasterizationState::CullOptions cullOptions);
            static D3D12_COMPARISON_FUNC dx12ComparisonFunction(Operations::ComparisonFunction comparisonFunction);
            static D3D12_DEPTH_STENCILOP_DESC dx12StencilOpDesc(StencilOpState state);
            static D3D12_STENCIL_OP dx12StencilOp(Operations::StencilOperation operation);
            static std::vector<DXGI_FORMAT> dx12GraphicsType(GraphicsType type);
            static uint32_t dx12FormatSize(DXGI_FORMAT format);
            static D3D12_FILTER dx12Filter(Sampler::Filter minFilter, Sampler::Filter magFilter, Sampler::Filter mipMapFilter, bool ansitrophyEnabled);
            static D3D12_TEXTURE_ADDRESS_MODE dx12AddressMode(Sampler::AddressMode mode);

            DX12Backend(SlagInitInfo initInfo);
            virtual ~DX12Backend()override;
            virtual void postGraphicsCardChosenSetup()override;
            virtual void preGraphicsCardDestroyCleanup()override;

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
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)override;
            virtual Texture* newTexture(Pixels::Format texelFormat, Texture::Type type, Texture::UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount, void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)override;
            //Buffers
            virtual Buffer* newBuffer(size_t size, Buffer::Accessibility accessibility, Buffer::UsageFlags usage)override;
            virtual Buffer* newBuffer(void* data, size_t dataSize, Buffer::Accessibility accessibility,Buffer::UsageFlags usage)override;
            //BufferViews
            virtual BufferView* newBufferView(Buffer* buffer, Pixels::Format format, uint64_t offset, uint64_t size)override;
            //swapchains
            virtual SwapChain* newSwapChain(PlatformData platformData, uint32_t width, uint32_t height,SwapChain::PresentMode presentMode, uint8_t desiredBackbufferCount,Pixels::Format format, SwapChain::AlphaCompositing compositing,FrameResources*(*createResourceFunction)(uint8_t frameIndex, SwapChain* inChain),void (*swapchainRebuiltFunction)(SwapChain* swapChain))override;
            //samplers
            virtual Sampler* newSampler(SamplerParameters parameters)override;
            //shaders
            virtual std::vector<ShaderCode::CodeLanguage> acceptedLanuages()override;
            virtual ShaderPipeline* newShaderPipeline(ShaderCode** shaders, uint32_t shaderCount, ShaderProperties& properties, VertexDescription& vertexDescription, FrameBufferDescription& framebufferDescription,std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)override;
            virtual ShaderPipeline* newShaderPipeline(const ShaderCode& computeShader, std::string(*rename)(const DescriptorRenameParameters&,void*), void* renameData)override;
            //descriptor pools
            virtual DescriptorPool* newDescriptorPool()override;
            virtual DescriptorPool* newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)override;
            //descriptor bundles
            virtual void setDescriptorBundleSampler(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, Sampler* sampler)override;
            virtual void setDescriptorBundleSampledTexture(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, Texture* texture)override;
            virtual void setDescriptorBundleStorageTexture(DescriptorBundle& descriptor, DescriptorIndex* index,uint32_t arrayElement, Texture* texture)override;
            virtual void setDescriptorBundleUniformTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)override;
            virtual void setDescriptorBundleStorageTexelBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, BufferView* bufferView)override;
            virtual void setDescriptorBundleUniformBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)override;
            virtual void setDescriptorBundleStorageBuffer(DescriptorBundle& descriptor, DescriptorIndex* index, uint32_t arrayElement, Buffer* buffer, uint64_t offset, uint64_t length)override;
            //Pixel Properties
            virtual PixelFormatProperties pixelFormatProperties(Pixels::Format format)override;

            void(* _dx12DebugHandler)(const std::string& message, SlagDebugLevel debugLevel, int32_t messageID)=nullptr;

        private:
            bool _valid = false;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BACKEND_H
