#ifndef SLAG_DX12RESOURCEDESCRIPTORHEAP_H
#define SLAG_DX12RESOURCEDESCRIPTORHEAP_H
#include <slag/Slag.h>
#include <directx/d3d12.h>

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;
        class DX12ResourceDescriptorHeap: public ResourceDescriptorHeap
        {
        public:
            DX12ResourceDescriptorHeap(DX12GraphicsCard* graphicsCard, uint32_t descriptorCount);
            DX12ResourceDescriptorHeap(const DX12ResourceDescriptorHeap&) = delete;
            DX12ResourceDescriptorHeap& operator=(const DX12ResourceDescriptorHeap&) = delete;
            DX12ResourceDescriptorHeap(DX12ResourceDescriptorHeap&& from) noexcept;
            DX12ResourceDescriptorHeap& operator=(DX12ResourceDescriptorHeap&& from) noexcept;
            ~DX12ResourceDescriptorHeap()override;
            
            GraphicsCard* graphicsCard()override;
            uint64_t size()override;
            void* data() override;
            uint64_t deviceAddress() override;

            ID3D12DescriptorHeap* dx12Handle() const;
        private:
            void move(DX12ResourceDescriptorHeap& from);
            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12DescriptorHeap* _heap = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12RESOURCEDESCRIPTORHEAP_H