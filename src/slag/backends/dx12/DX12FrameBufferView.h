#ifndef SLAG_DX12FRAMEBUFFERVIEW_H
#define SLAG_DX12FRAMEBUFFERVIEW_H
#include <slag/Slag.h>

#include "../../../../cmake-build-release/_deps/directx-headers-src/include/directx/d3d12.h"

namespace slag
{
    namespace dx12
    {
        class DX12GraphicsCard;

        class DX12FrameBufferView: public FrameBufferView
        {
        public:
            DX12FrameBufferView(DX12GraphicsCard* graphicsCard, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount);
            DX12FrameBufferView(const DX12FrameBufferView&)=delete;
            DX12FrameBufferView& operator=(const DX12FrameBufferView&)=delete;
            DX12FrameBufferView(DX12FrameBufferView&& from) noexcept;
            DX12FrameBufferView& operator=(DX12FrameBufferView&& from) noexcept;
            ~DX12FrameBufferView()override;
            Texture* texture()override;
            uint32_t mip()override;
            uint32_t baseLayer()override;
            uint32_t layerCount()override;
            GraphicsCard* graphicsCard()override;
            D3D12_CPU_DESCRIPTOR_HANDLE dx12Handle() const;
        private:
            void move(DX12FrameBufferView& from);
            DX12GraphicsCard* _graphicsCard=nullptr;
            Texture* _texture=nullptr;
            ID3D12DescriptorHeap* _dx12Heap=nullptr;
            uint32_t _mip=0;
            uint32_t _baseLayer=0;
            uint32_t _layerCount=0;
        };
    } // dx12
} // slag

#endif //SLAG_DX12FRAMEBUFFERVIEW_H