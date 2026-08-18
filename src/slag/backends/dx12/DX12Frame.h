#ifndef SLAG_DX12FRAME_H
#define SLAG_DX12FRAME_H
#include <slag/Slag.h>
#include <directx/d3d12.h>

#include "DX12FrameBufferView.h"

namespace slag
{
    namespace dx12
    {
        class DX12SwapChain;
        class DX12Texture;
        class DX12Frame: public Frame
        {
        public:
            DX12Frame(uint32_t frameIndex, ID3D12Resource* texture,uint32_t width, uint32_t height, PixelFormat format, DX12SwapChain* parentChain);
            ~DX12Frame()override;
            DX12Frame(const DX12Frame&) = delete;
            DX12Frame& operator=(const DX12Frame&) = delete;
            DX12Frame(DX12Frame&& from) noexcept;
            DX12Frame& operator=(DX12Frame&& from) noexcept;
            [[nodiscard]] Texture* renderBuffer()override;
            [[nodiscard]] FrameBufferView* defaultView()override;
        private:
            void move(DX12Frame& from);
            uint32_t _frameIndex = 0;
            DX12Texture* _renderBuffer = nullptr;
            DX12FrameBufferView* _frameBufferView = nullptr;
            DX12SwapChain* _parentChain = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12FRAME_H