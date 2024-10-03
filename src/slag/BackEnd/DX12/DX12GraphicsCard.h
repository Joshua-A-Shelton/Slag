#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <directx/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "../../GraphicsCard.h"
#include "DX12Queue.h"
#include "D3D12MemAlloc.h"
#include <queue>

namespace slag
{
    namespace dx
    {

        class DX12GraphicsCard: public GraphicsCard
        {
        public:
            DX12GraphicsCard(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter, Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory);
            ~DX12GraphicsCard()override;
            Microsoft::WRL::ComPtr<ID3D12Device2>& device();
            Microsoft::WRL::ComPtr<IDXGIFactory4>& dxgiFactory();

            GpuQueue* graphicsQueue()override;
            GpuQueue* transferQueue()override;
            GpuQueue* computeQueue()override;

            D3D12MA::Allocator* allocator();
            D3D12_CPU_DESCRIPTOR_HANDLE getSamplerHandle();
            void freeSamplerHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle);
            ID3D12DescriptorHeap* samplerHeap();
            bool supportsEnhancedBarriers();

            void defragmentMemory()override;
        private:
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
            DX12Queue* _graphics = nullptr;
            DX12Queue* _transfer = nullptr;
            DX12Queue* _compute = nullptr;
            D3D12MA::Allocator* _allocator = nullptr;
            ID3D12DescriptorHeap* _samplerHeap = nullptr;
            size_t _samplerIndex = 0;
            std::queue<D3D12_CPU_DESCRIPTOR_HANDLE> _freedSamplerHandles;

            bool _supportsEnhancedBarriers = false;
        };

    } // dx
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
