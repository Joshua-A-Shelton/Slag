#ifndef SLAG_DX12SWAPCHAIN_H
#define SLAG_DX12SWAPCHAIN_H
#include "../../Swapchain.h"
// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include <d3d12.h>
namespace slag
{
    namespace dx12
    {

        class DX12Swapchain: public Swapchain
        {
        public:
            ~DX12Swapchain()override;
            Frame* currentFrame()override;
            Frame* next()override;
            size_t backBufferCount()override;
            void backBufferCount(size_t count)override;
            bool vsyncEnabled()override;
            void vsyncEnabled(bool enabled)override;
            uint32_t width()override;
            uint32_t height()override;
            void resize(uint32_t width, uint32_t height)override;
            Pixels::PixelFormat imageFormat()override;
        private:
            IDXGISwapChain4* _swapchain;
        };

    } // slag
} // dx12

#endif //SLAG_DX12SWAPCHAIN_H
