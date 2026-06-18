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
            uint32_t descriptorCount()override;
            void setUniformTexture(uint32_t index, Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)override;
            void setUnorderedAccessTexture(uint32_t index, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)override;
            void setUniformStructuredBuffer(uint32_t index, Buffer* buffer, uint32_t offset, uint32_t length)override;
            void setStorageStructuredBuffer(uint32_t index, Buffer* buffer, uint64_t elementIndex, uint64_t elementCount, uint64_t elementStride)override;
            void setUniformTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)override;
            void setStorageTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format, uint64_t offset, uint64_t length)override;

            ID3D12DescriptorHeap* dx12Handle() const;
        private:
            void move(DX12ResourceDescriptorHeap& from);
            DX12GraphicsCard* _graphicsCard = nullptr;
            ID3D12DescriptorHeap* _heap = nullptr;
            uint32_t _descriptorSize = 0;
        };
    } // dx12
} // slag

#endif //SLAG_DX12RESOURCEDESCRIPTORHEAP_H