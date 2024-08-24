#ifndef SLAG_DX12GRAPHICSCARD_H
#define SLAG_DX12GRAPHICSCARD_H
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "../../GraphicsCard.h"
#include "DX12Queue.h"

namespace slag
{
    namespace dx
    {

        class DX12GraphicsCard: public GraphicsCard
        {
        public:
            DX12GraphicsCard(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter);
            ~DX12GraphicsCard();
            Microsoft::WRL::ComPtr<ID3D12Device2>& device();

            GpuQueue* GraphicsQueue()override;
            GpuQueue* TransferQueue()override;
            GpuQueue* ComputeQueue()override;
        private:
            Microsoft::WRL::ComPtr<ID3D12Device2> _device = nullptr;
            DX12Queue* _graphics = nullptr;
            DX12Queue* _transfer = nullptr;
            DX12Queue* _compute = nullptr;
        };

    } // dx
} // slag

#endif //SLAG_DX12GRAPHICSCARD_H
