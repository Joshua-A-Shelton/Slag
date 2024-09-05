#include "DX12Texture.h"
#include "DX12Lib.h"

namespace slag
{
    namespace dx
    {
        DX12Texture::DX12Texture(void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_RESOURCE_STATES initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            DX12CommandBuffer commandBuffer(GpuQueue::QueueType::Compute);
            commandBuffer.begin();
            build(&commandBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            commandBuffer.end();
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

            if(generateMips &&  mipLevels > 1)
            {
                updateMipMaps(onBuffer);
            }

        }

        uint32_t DX12Texture::width()
        {
            return _width;
        }

        uint32_t DX12Texture::height()
        {
            return _height;
        }

        uint32_t DX12Texture::mipLevels()
        {
            return _mipLevels;
        }

        ID3D12Resource* DX12Texture::texture()
        {
            return _texture;
        }

        void DX12Texture::updateMipMaps()
        {
            DX12CommandBuffer commandBuffer(GpuQueue::Compute);
            commandBuffer.begin();
            updateMipMaps(&commandBuffer);
            commandBuffer.end();
            DX12Lib::card()->computeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        void DX12Texture::updateMipMaps(DX12CommandBuffer* onBuffer)
        {
            switch (onBuffer->commandType())
            {
                case GpuQueue::Graphics:
                    throw std::runtime_error("Generating mip maps on Graphics Queue not implemented yet");
                    break;
                case GpuQueue::Compute:
                    throw std::runtime_error("Generating mip maps on Compute Queue not implemented yet");
                    break;
                case GpuQueue::Transfer:
                    throw std::runtime_error("Cannot generate mip maps on Transfer Queue");
                    break;
            }
        }


    } // dx
} // slag