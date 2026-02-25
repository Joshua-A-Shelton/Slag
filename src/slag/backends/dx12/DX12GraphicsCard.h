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
            ~DX12GraphicsCard();
            DX12GraphicsCard(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard& operator=(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard(DX12GraphicsCard&& from);
            DX12GraphicsCard& operator=(DX12GraphicsCard&& from);

            [[nodiscard]] std::string name()const override;
            [[nodiscard]] uint64_t videoMemory()const override;
            [[nodiscard]] uint64_t maxShaderAccessUniformBufferSize()const override;
            [[nodiscard]] PixelFormatProperties formatProperties(PixelFormat format)const override;
            [[nodiscard]] bool cacheCoherentSharedMemory()const override;
            [[nodiscard]] SubmissionQueue* graphicsQueue()override;
            [[nodiscard]] SubmissionQueue* computeQueue()override;
            [[nodiscard]] SubmissionQueue* transferQueue()override;
            uint64_t defragmentMemory(SemaphoreValue* waitFor, uint32_t waitCount, SemaphoreValue* signal, uint32_t signalCount, uint64_t targetBytes=0)override;
            //Command Buffers
            [[nodiscard]] CommandBuffer* newCommandBuffer(QueueType type)override;
            //Semaphores
            [[nodiscard]] Semaphore* newSemaphore(uint64_t initialValue=0)override;
            //Buffers
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferMemoryType memoryType = BufferMemoryType::GENERAL,
                BufferCPUAccess cpuAccess = BufferCPUAccess::WRITE_ONLY)override;
            //Textures
            [[nodiscard]] Texture* newTexture(
            uint32_t width,
            PixelFormat format,
            TextureUsageFlags usage,
            uint32_t mipLevels,
            uint32_t layers)override;

            [[nodiscard]] Texture* newTexture(
                uint32_t width,
                uint32_t height,
                PixelFormat format,
                TextureUsageFlags usage,
                uint32_t mipLevels,
                SampleCount sampleCount,
                uint32_t layers)override;

            [[nodiscard]] Texture* newTexture(
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
                SampleCount sampleCount,
                uint32_t arrayDepth
                )override;

            //DX12 specific features
            D3D12MA::Allocator* allocator();
            Microsoft::WRL::ComPtr<ID3D12Device2>& device();
        private:
            void move(DX12GraphicsCard& from);
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter4 = nullptr;
            uint64_t _videoMemory = 0;
            D3D12MA::Allocator* _allocator = nullptr;
            DX12SubmissionQueue* _graphicsQueue = nullptr;
            DX12SubmissionQueue* _computeQueue = nullptr;
            DX12SubmissionQueue* _transferQueue = nullptr;
            bool _sharedMemory = false;
        };
    } // dx12
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
