#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <slag/Slag.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>
namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard: public GraphicsCard
        {
        public:
            DX12GraphicsCard(Microsoft::WRL::ComPtr<ID3D12Device2> device, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory,Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter, bool includeDebugHandling);
            ~DX12GraphicsCard()=default;
            DX12GraphicsCard(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard& operator=(const DX12GraphicsCard&) = delete;
            DX12GraphicsCard(DX12GraphicsCard&& from);
            DX12GraphicsCard& operator=(DX12GraphicsCard&& from);

            [[nodiscard]] std::string name()const override;
            [[nodiscard]] uint64_t videoMemory()const override;
            [[nodiscard]] bool cacheCoherentSharedMemory()const override;
            //Buffers
            /**
             * Allocate a new buffer
             * @param size Size in bytes of the buffer
             * @param usage Kind of data will the buffer store
             * @param shaderAccess Data access permissions for shaders
             * @param cpuAccess Data access permissions for the cpu
             * @return
             */
            [[nodiscard]] Buffer* newBuffer(
                uint64_t size,
                BufferUsage usage = BufferUsage::ARBITRARY,
                BufferShaderAccess shaderAccess = BufferShaderAccess::READ_WRITE,
                BufferCPUAccess cpuAccess = BufferCPUAccess::WRITE_ONLY)override;
        private:
            void move(DX12GraphicsCard& from);
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            Microsoft::WRL::ComPtr<IDXGIAdapter4> _dxgiAdapter4 = nullptr;
            uint64_t _videoMemory = 0;
            bool _sharedMemory = false;
        };
    } // dx12
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
