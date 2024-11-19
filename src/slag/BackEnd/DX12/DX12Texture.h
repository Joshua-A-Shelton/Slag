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

        class DX12CommandBuffer;
        class DX12Texture: public Texture, resources::Resource
        {
        public:
            DX12Texture(ID3D12Resource* dx12Texture, bool ownTexture, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t  mipLevels, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately);

            DX12Texture(void** texelDataArray, size_t texelDataCount, uint64_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, Texture::Layout initializedLayout, bool destroyImmediately);
            DX12Texture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately);
            ~DX12Texture();
            DX12Texture(const DX12Texture&)=delete;
            DX12Texture& operator=(const DX12Texture&)=delete;
            DX12Texture(DX12Texture&& from);
            DX12Texture& operator=(DX12Texture&& from);
            Type type()override;
            uint32_t width()override;
            uint32_t height()override;
            uint32_t layers()override;
            uint32_t mipLevels()override;
            uint8_t sampleCount()override;
            ID3D12Resource* texture();
            DXGI_FORMAT underlyingFormat();
            D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle();

        private:
            void move(DX12Texture&& from);
            void construct(Pixels::Format dataFormat, Texture::Type textureType, uint32_t width, uint32_t height,uint32_t layers, uint32_t mipLevels, uint8_t samples, D3D12_RESOURCE_FLAGS usage, Texture::Layout initialLayout);
            ID3D12Resource* _texture = nullptr;
            D3D12MA::Allocation* _allocation = nullptr;
            ID3D12DescriptorHeap* _heap = nullptr;
            D3D12_CPU_DESCRIPTOR_HANDLE _view{};
            Texture::Type _type;
            uint32_t _width = 0;
            uint32_t _height = 0;
            uint32_t _layers=1;
            uint32_t _mipLevels=1;
            uint8_t _sampleCount=1;
            DXGI_FORMAT _format;
            D3D12_RESOURCE_FLAGS _usage;

        };

    } // dx
} // slag

#endif //SLAG_DX12TEXTURE_H
