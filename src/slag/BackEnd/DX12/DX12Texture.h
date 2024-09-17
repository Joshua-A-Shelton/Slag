#ifndef SLAG_DX12TEXTURE_H
#define SLAG_DX12TEXTURE_H

#include <directx/d3d12.h>
#include "../../Texture.h"
#include "../../Resources/Resource.h"
#include "DX12CommandBuffer.h"
#include "D3D12MemAlloc.h"

namespace slag
{
    namespace dx
    {

        class DX12Texture: public Texture, resources::Resource
        {
        public:
            DX12Texture(ID3D12Resource* dx12Texture, bool ownTexture, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t  mipLevels, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately);
            DX12Texture(void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips, bool destroyImmediately);
            DX12Texture(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips, bool destroyImmediately);
            ~DX12Texture();
            DX12Texture(const DX12Texture&)=delete;
            DX12Texture& operator=(const DX12Texture&)=delete;
            DX12Texture(DX12Texture&& from);
            DX12Texture& operator=(DX12Texture&& from);
            void* gpuID()override;
            uint32_t width()override;
            uint32_t height()override;
            uint32_t mipLevels()override;
            void updateMipMaps();
            void updateMipMaps(DX12CommandBuffer* onBuffer);
            ID3D12Resource* texture();
        private:
            void move(DX12Texture&& from);
            void build(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips);
            ID3D12Resource* _texture = nullptr;
            D3D12MA::Allocation* _allocation = nullptr;

            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _mipLevels=1;
            DXGI_FORMAT _format;
            D3D12_RESOURCE_FLAGS _usage;
        };

    } // dx
} // slag

#endif //SLAG_DX12TEXTURE_H
