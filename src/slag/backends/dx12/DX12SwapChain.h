#ifndef SLAG_DX12SWAPCHAIN_H
#define SLAG_DX12SWAPCHAIN_H
#include <dxgi1_5.h>
#include <slag/Slag.h>

#include "DX12Frame.h"
#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        class DX12SwapChain: public SwapChain
        {
        public:
            DX12SwapChain(DX12GraphicsCard* graphicsCard, PlatformData platformData, uint32_t width, uint32_t height, const SwapChainParameters& parameters);
            ~DX12SwapChain()override;
            DX12SwapChain(const DX12SwapChain&) = delete;
            DX12SwapChain& operator=(const DX12SwapChain&) = delete;
            DX12SwapChain(DX12SwapChain&& from) noexcept;
            DX12SwapChain& operator=(DX12SwapChain&& from) noexcept;
            Frame* next()override;
            [[nodiscard]] Frame* currentFrame()override;
            void present()override;
            [[nodiscard]] const SwapChainParameters& parameters()const override;
            GraphicsCard* graphicsCard()override;

        private:
            void move(DX12SwapChain& from);
            void rebuild();
            HWND _window = nullptr;
            DX12GraphicsCard* _graphicsCard = nullptr;
            Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
            SwapChainParameters _parameters{};
            std::vector<DX12Frame> _frames;
            uint32_t _width = 0;
            uint32_t _height = 0;
            bool _presentRequired = false;
        };

    } // dx12
} // slag

#endif //SLAG_DX12SWAPCHAIN_H