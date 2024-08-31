#include "DX12Texture.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {
        DX12Texture::DX12Texture(void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            DX12CommandBuffer commandBuffer(D3D12_COMMAND_LIST_TYPE_COMPUTE);
            build(&commandBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            DX12Lib::card()->computeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        DX12Texture::DX12Texture(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            build(onBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
        }

        DX12Texture::~DX12Texture()
        {
            if(_allocation)
            {
                smartDestroy();
            }
        }

        DX12Texture::DX12Texture(DX12Texture&& from): resources::Resource(from._destroyImmediately)
        {
            move(std::move(from));
        }

        DX12Texture& DX12Texture::operator=(DX12Texture&& from)
        {
            move(std::move(from));
            return *this;
        }

        void DX12Texture::move(DX12Texture&& from)
        {
            resources::Resource::move(from);
            std::swap(_allocation,from._allocation);
        }

        void* DX12Texture::gpuID()
        {
            return _allocation;
        }

        void DX12Texture::build(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips)
        {
            _width = width;
            _height = height;
            _mipLevels = mipLevels;

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = mipLevels;
            resourceDesc.Format = textureFormat;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = usage;

            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            DX12Lib::card()->allocator()->CreateResource(&allocationDesc,&resourceDesc,initializedLayout,nullptr,&_allocation, IID_PPV_ARGS(&_texture));

            auto tex = _texture;
            auto alloc = _allocation;
            _disposeFunction = [=]()
            {
                tex->Release();
                alloc->Release();
            };

            void* mappedPtr;
            _texture->Map(0, nullptr, &mappedPtr);

            memcpy(mappedPtr, texelData, dataSize);

            _texture->Unmap(0, nullptr);

        }

        ID3D12Resource* DX12Texture::texture()
        {
            return _texture;
        }
    } // dx
} // slag