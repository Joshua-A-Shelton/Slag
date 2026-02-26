#ifndef SLAG_DX12BACKEND_H
#define SLAG_DX12BACKEND_H
#include <string>
#include <vector>

#include "DX12GraphicsCard.h"
#include "slag/Slag.h"
#include "slag/core/IBackend.h"
using SlagDebugHandlerPtr = void(*)(const std::string&,slag::DebugLevel,int32_t);
namespace slag
{
    namespace dx12
    {
        class DX12Backend: public IBackend
        {
        public:
            DX12Backend();
            ~DX12Backend()override=default;
            [[nodiscard]] BackendAPI api()const override;
            [[nodiscard]] uint32_t graphicsCardCount()const override;
            [[nodiscard]] GraphicsCard* graphicsCard(uint32_t index)override;
            [[nodiscard]] SlagDebugHandlerPtr getDebugHandler() const;

            static DXGI_FORMAT nativeFormat(PixelFormat format);
            static D3D12_RESOURCE_FLAGS nativeTextureUsageFlags(TextureUsageFlags usage);
        private:
            SlagInitializationResult initializeBackend(const InitializationData& initializationData)override;
            std::vector<DX12GraphicsCard> _graphicsCards;
            void(*_debugHandler)(const std::string& message,DebugLevel debugLevel, int32_t messageID)=nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BACKEND_H
