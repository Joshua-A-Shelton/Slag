#ifndef SLAG_DX12FRAME_H
#define SLAG_DX12FRAME_H
#include "../../Frame.h"
#include "DX12Texture.h"

namespace slag
{
    namespace dx
    {
        class DX12Swapchain;
        class DX12Frame: public Frame
        {
        public:
            DX12Frame(ID3D12Resource* texture, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS textureUsage, DX12Swapchain* from);
            ~DX12Frame()override;
            DX12Frame(const DX12Frame&)=delete;
            DX12Frame& operator=(const DX12Frame&)=delete;
            DX12Frame(DX12Frame&& from);
            DX12Frame& operator=(DX12Frame&& from);
            Texture* backBuffer()override;
            CommandBuffer* commandBuffer()override;
            void begin()override;
            void end()override;
        private:
            void move(DX12Frame&& from);
            DX12Texture* _backBuffer = nullptr;
            DX12CommandBuffer* _commandBuffer = nullptr;
            DX12Swapchain* _from = nullptr;
        };
    } // dx
} // slag

#endif //SLAG_DX12FRAME_H
