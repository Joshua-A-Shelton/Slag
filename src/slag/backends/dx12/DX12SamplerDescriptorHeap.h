#ifndef SLAG_DX12SAMPLERDESCRIPTORHEAP_H
#define SLAG_DX12SAMPLERDESCRIPTORHEAP_H
#include <slag/Slag.h>

#include "directx/d3d12.h"

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class DX12SamplerDescriptorHeap: public SamplerDescriptorHeap
        {
        public:
            DX12SamplerDescriptorHeap(DX12GraphicsCard* graphicsCard, uint32_t descriptorCount);
            DX12SamplerDescriptorHeap(const DX12SamplerDescriptorHeap&) = delete;
            DX12SamplerDescriptorHeap& operator=(const DX12SamplerDescriptorHeap&) = delete;
            DX12SamplerDescriptorHeap(DX12SamplerDescriptorHeap&& from) noexcept;
            DX12SamplerDescriptorHeap& operator=(DX12SamplerDescriptorHeap&& from) noexcept;
            ~DX12SamplerDescriptorHeap()override;

            void setSampler(uint32_t index, Sampler* sampler)override;
            GraphicsCard* graphicsCard()override;
            uint32_t descriptorCount()override;
            ID3D12DescriptorHeap* dx12Handle() const;
        private:
            void move(DX12SamplerDescriptorHeap& from);
            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12DescriptorHeap* _heap = nullptr;
            uint32_t _descriptorSize = 0;
        };
    } // dx12
} // slag

#endif //SLAG_DX12SAMPLERDESCRIPTORHEAP_H