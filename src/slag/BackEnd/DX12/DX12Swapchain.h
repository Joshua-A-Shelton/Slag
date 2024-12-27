#ifndef SLAG_DX12SWAPCHAIN_H
#define SLAG_DX12SWAPCHAIN_H
#include <directx/d3d12.h>
#include <dxgi1_5.h>
#include <wrl/client.h>
#include "../../Swapchain.h"
#include "DX12Frame.h"

namespace slag
{
    namespace dx
    {

        class DX12Swapchain: public Swapchain
        {
        public:
            DX12Swapchain(PlatformData platformData, uint32_t width, uint32_t height, uint8_t backBuffers, PresentMode mode, Pixels::Format format, FrameResources* (*createResourceFunction)(size_t frameIndex, Swapchain* inChain));
            ~DX12Swapchain()override;
            DX12Swapchain(const DX12Swapchain&)=delete;
            DX12Swapchain& operator=(const DX12Swapchain&)=delete;
            DX12Swapchain(DX12Swapchain&& from);
            DX12Swapchain& operator=(DX12Swapchain&& from);
            Frame* next()override;
            Frame* nextIfReady()override;
            Frame* currentFrame()override;
            uint8_t currentFrameIndex()override;
            uint8_t backBuffers()override;
            void backBuffers(uint8_t count)override;
            uint32_t width()override;
            uint32_t height()override;
            void resize(uint32_t width, uint32_t height)override;
            PresentMode presentMode()override;
            void presentMode(PresentMode mode)override;
            IDXGISwapChain4* underlyingSwapchain();

        private:
            void move(DX12Swapchain&& from);
            void rebuild();
            HWND _surface= nullptr;
            Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapchain = nullptr;
            uint32_t _width=0;
            uint32_t _height=0;
            uint8_t _backBuffers=0;
            PresentMode _presentMode = PresentMode::MAILBOX;
            Pixels::Format _format;

            std::vector<DX12Frame> _frames;
        };

    } // dx
} // slag

#endif //SLAG_DX12SWAPCHAIN_H
