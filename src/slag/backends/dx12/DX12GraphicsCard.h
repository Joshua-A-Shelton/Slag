#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <slag/Slag.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
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
            [[nodiscard]] const DescriptorTableDetails& descriptorTableDetails()const override;
            [[nodiscard]] PixelFormatProperties formatProperties(PixelFormat format)const override;
            [[nodiscard]] SubmissionQueue* graphicsQueue()override;
            [[nodiscard]] SubmissionQueue* computeQueue()override;
            [[nodiscard]] SubmissionQueue* transferQueue()override;
            uint64_t defragmentMemory(uint64_t targetBytes,std::function<void(MemoryReference*)> memoryMoved)override;
            //Shaders
            [[nodiscard]] ShaderModule* newShaderModule(ShaderLanguage language, void* data, uint32_t dataLength)override;
            [[nodiscard]] ShaderPipeline* newShaderPipeline(
                const VertexDescription& vertexDescription,
                ShaderModule* vertexShader,
                ShaderModule* fragmentShader,
                const PipelineState& pipelineState,
                const FramebufferDescription& framebufferDescription)override;
            //Command Buffers
            [[nodiscard]] CommandBuffer* newCommandBuffer(QueueType type)override;
            //Semaphores
            [[nodiscard]] Semaphore* newSemaphore(uint64_t initialValue)override;
            //Buffers
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferCPUAccess cpuAccess,
                BufferMemoryType memoryType)override;
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

            //DX12 specific features
            [[nodiscard]] D3D12MA::Allocator* allocator() const;
            [[nodiscard]] D3D12MA::Pool* cpuReadablePool() const;
            Microsoft::WRL::ComPtr<ID3D12Device2>& device();
        private:
            void move(DX12GraphicsCard& from);
            GraphicsCardMemoryProperties _memoryProperties{};
            GraphicsCardCapabilities _capabilities{};
            DescriptorTableDetails _descriptorTableDetails{};
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter4 = nullptr;
            D3D12MA::Allocator* _allocator = nullptr;
            DX12SubmissionQueue* _graphicsQueue = nullptr;
            DX12SubmissionQueue* _computeQueue = nullptr;
            DX12SubmissionQueue* _transferQueue = nullptr;
            D3D12MA::Pool* _cpuReadablePool = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
