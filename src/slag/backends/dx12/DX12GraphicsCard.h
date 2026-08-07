#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <slag/Slag.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <mutex>
#include <queue>
#include <wrl/client.h>

namespace slag::dx12
{
    class DX12SubmissionQueue;
}

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard: public GraphicsCard
        {
        public:
            DX12GraphicsCard(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory,Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter, bool includeDebugHandling);
            ~DX12GraphicsCard()override;
            DX12GraphicsCard(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard& operator=(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard(DX12GraphicsCard&& from) noexcept;
            DX12GraphicsCard& operator=(DX12GraphicsCard&& from) noexcept;

            [[nodiscard]] std::string name()const override;
            [[nodiscard]] const GraphicsCardMemoryProperties& memoryProperties()const override;
            [[nodiscard]] const GraphicsCardCapabilities& capabilities()const override;
            [[nodiscard]] const DescriptorHeapDetails& descriptorHeapDetails()const override;
            [[nodiscard]] PixelFormatProperties formatProperties(PixelFormat format)const override;
            [[nodiscard]] SubmissionQueue* graphicsQueue()override;
            [[nodiscard]] SubmissionQueue* computeQueue()override;
            [[nodiscard]] SubmissionQueue* transferQueue()override;
            uint64_t defragmentMemory(uint64_t targetBytes,std::function<void(MemoryReference*)> memoryMoved)override;
            SwapChain* newSwapchain(const PlatformData& platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters)override;
            //Shaders
            [[nodiscard]] ShaderPipeline* newShaderPipeline(
                const VertexDescription& vertexDescription,
                const ShaderCode& vertexShader,
                const ShaderCode& fragmentShader,
                const PipelineState& pipelineState,
                const FramebufferDescription& framebufferDescription)override;

            [[nodiscard]] ShaderPipeline* newShaderPipeline(ShaderCode* computeShader)override;

            //Command Buffers
            [[nodiscard]] CommandBuffer* newCommandBuffer(QueueType type)override;
            //Semaphores
            [[nodiscard]] Semaphore* newSemaphore(uint64_t initialValue)override;
            //Buffers
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferCPUAccess cpuAccess,
                BufferMemoryType memoryType)override;

            [[nodiscard]] ResourceDescriptorHeap* newResourceDescriptorHeap(uint32_t minDescriptorCount)override;

            [[nodiscard]] SamplerDescriptorHeap* newSamplerDescriptorHeap(uint32_t descriptorCount)override;

            void writeUniformBufferDescriptor(Buffer* buffer, uint64_t offset, uint64_t length, void* destination)override;
            void writeReadWriteBufferDescriptor(Buffer* buffer, uint64_t firstElementIndex,uint64_t elementCount, uint64_t elementStride, void* destination)override;
            void writeUniformTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex, uint64_t elementCount, void* destination)override;
            void writeReadWriteTexelBuffer(Buffer* buffer, PixelFormat format, uint64_t firstElementIndex, uint64_t elementCount, void* destination)override;
            void writeUniformTextureDescriptor(Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount, void* destination)override;
            void writeReadWriteTextureDescriptor(Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount, void* destination)override;
            void writeSamplerDescriptor(Sampler* sampler, void* destination)override;

            //Textures
            [[nodiscard]] Texture* newTexture1D(
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)override;

            [[nodiscard]] Texture* newTexture2D(
                uint32_t width,
                uint32_t height,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                SampleCount sampleCount,
                uint32_t layers)override;

            [[nodiscard]] Texture* newTexture3D(
                uint32_t width,
                uint32_t height,
                uint32_t depth,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels)override;

            [[nodiscard]] Texture* newTextureCube(
                uint32_t dimension,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                uint32_t arrayDepth
                )override;

            [[nodiscard]] Sampler* newSampler(
                SamplerFilter min,
                SamplerFilter mag,
                SamplerFilter mip,
                SamplerAddressMode u,
                SamplerAddressMode v,
                SamplerAddressMode w,
                float mipLODBias,
                bool anisotrophyEnabled,
                uint8_t  maxAnisotrophy,
                ComparisonFunction comparisonFunction,
                Color borderColor,
                float minLOD,
                float maxLOD
                )override;

            //DX12 specific features
            [[nodiscard]] D3D12MA::Allocator* allocator() const;
            [[nodiscard]] D3D12MA::Pool* cpuReadablePool() const;
            Microsoft::WRL::ComPtr<ID3D12Device2>& device();
            Microsoft::WRL::ComPtr<IDXGIFactory4>& dxgiFactory();
            ID3D12RootSignature* rootSignature() const;
            ID3D12CommandSignature* drawIndirectCommandSignature() const;
            ID3D12CommandSignature* drawIndexedIndirectCommandSignature() const;
            ID3D12CommandSignature* dispatchIndirectCommandSignature() const;

        private:
            void move(DX12GraphicsCard& from);
            GraphicsCardMemoryProperties _memoryProperties{};
            GraphicsCardCapabilities _capabilities{};
            DescriptorHeapDetails _descriptorHeapDetails{};
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter4 = nullptr;
            D3D12MA::Allocator* _allocator = nullptr;
            DX12SubmissionQueue* _graphicsQueue = nullptr;
            DX12SubmissionQueue* _computeQueue = nullptr;
            DX12SubmissionQueue* _transferQueue = nullptr;
            D3D12MA::Pool* _cpuReadablePool = nullptr;
            ID3D12RootSignature* _rootSignature = nullptr;

            ID3D12CommandSignature* _drawIndirectCommandSignature = nullptr;
            ID3D12CommandSignature* _drawIndexedIndirectCommandSignature = nullptr;
            ID3D12CommandSignature* _dispatchIndirectCommandSignature = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
