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
            [[nodiscard]] uint32_t supportedShaderLanguageCount()const override;
            [[nodiscard]] ShaderLanguage supportedShaderLanguage(uint32_t index)const override;
            [[nodiscard]] SlagDebugHandlerPtr getDebugHandler() const;

            static DXGI_FORMAT nativeFormat(PixelFormat format);
            static D3D12_RESOURCE_FLAGS nativeTextureUsageFlags(TextureUsageFlags usage);
            static D3D12_BARRIER_ACCESS nativeMemoryCaches(MemoryCaches caches);
            static D3D12_BARRIER_SYNC nativePipelineStages(SyncStages stages);
            static D3D12_BLEND nativeBlendFactor(BlendFactor factor);
            static D3D12_BLEND_OP nativeBlendOp(BlendOperation op);
            static D3D12_LOGIC_OP nativeLogicOp(LogicOperation op);
            static D3D12_FILL_MODE nativeFillMode(RasterizationState::DrawMode mode);
            static D3D12_CULL_MODE nativeCullMode(RasterizationState::CullOptions mode);
            static D3D12_COMPARISON_FUNC nativeCompareFunc(ComparisonFunction func);
            static D3D12_DEPTH_STENCILOP_DESC nativeDepthStencilOpDesc(StencilOpState opstate);
            static D3D12_FILTER nativeFilter(SamplerFilter min, SamplerFilter mag, SamplerFilter mip, bool anisotrophyEnabled);
            static D3D12_TEXTURE_ADDRESS_MODE nativeAddressMode(SamplerAddressMode mode);

            static D3D12_SRV_DIMENSION nativeSRVTextureDimension(TextureType type, uint32_t arraySize, SampleCount sampleCount);
            static D3D12_UAV_DIMENSION nativeUAVTextureDimension(TextureType type, uint32_t arraySize, SampleCount sampleCount);

            static D3D12_BARRIER_LAYOUT nativeImageLayout(TextureLayout layout);
        protected:
            SlagInitializationResult initializeBackend(const InitializationData& initializationData)override;
        private:
            std::vector<DX12GraphicsCard> _graphicsCards;
            void(*_debugHandler)(const std::string& message,DebugLevel debugLevel, int32_t messageID)=nullptr;
            Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory = nullptr;
        };
    } // dx12
} // slag

#endif //SLAG_DX12BACKEND_H
