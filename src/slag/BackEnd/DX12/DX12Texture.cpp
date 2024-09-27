#include <cassert>
#include "DX12Texture.h"
#include "DX12Lib.h"
#include "DX12Buffer.h"

namespace slag
{
    namespace dx
    {
        DX12Texture::DX12Texture(ID3D12Resource* dx12Texture, bool ownTexture, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            assert(!(usage & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && usage & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && "Texture cannot be both render target and depth stencil");
            _texture = dx12Texture;
            _format = textureFormat;
            _width = width;
            _height = height;
            _mipLevels = mipLevels;
            _usage = usage;

            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            if(usage & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                DX12Lib::card()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12Lib::card()->device()->CreateRenderTargetView(_texture, nullptr,_view);
            }
            else if(usage & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
                DX12Lib::card()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12Lib::card()->device()->CreateDepthStencilView(_texture, nullptr,_view);
            }


            auto heap = _heap;
            if(ownTexture)
            {
                _disposeFunction = [=]{dx12Texture->Release(); if(heap)(heap->Release());};
            }
            else
            {
                _disposeFunction = [=]{if(heap)(heap->Release());};
            }
        }

        DX12Texture::DX12Texture(void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_BARRIER_LAYOUT  initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            DX12CommandBuffer commandBuffer(GpuQueue::QueueType::Compute);
            commandBuffer.begin();
            build(&commandBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            commandBuffer.end();
            DX12Lib::card()->computeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        DX12Texture::DX12Texture(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_BARRIER_LAYOUT initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
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
            std::swap(_texture,from._texture);
            std::swap(_allocation,from._allocation);
            std::swap(_heap,from._heap);

            _view = from._view;
            _width = from._width;
            _height = from._height;
            _mipLevels = from._mipLevels;
            _format = from._format;
            _usage = from._usage;
        }

        void* DX12Texture::gpuID()
        {
            return _allocation;
        }

        void DX12Texture::build(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, D3D12_BARRIER_LAYOUT initializedLayout, bool generateMips)
        {
            assert(!(usage & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && usage & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && "Texture cannot be both render target and depth stencil");
            _width = width;
            _height = height;
            _mipLevels = mipLevels;
            _format = textureFormat;
            _usage = usage;

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

            DX12Lib::card()->allocator()->CreateResource(&allocationDesc,&resourceDesc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,&_allocation, IID_PPV_ARGS(&_texture));

            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            if(usage & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                DX12Lib::card()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12Lib::card()->device()->CreateRenderTargetView(_texture, nullptr,_view);
            }
            else if(usage & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
                DX12Lib::card()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12Lib::card()->device()->CreateDepthStencilView(_texture, nullptr,_view);
            }

            auto tex = _texture;
            auto alloc = _allocation;
            auto heap = _heap;
            _disposeFunction = [=]()
            {
                tex->Release();
                alloc->Release();
                if(heap)(heap->Release());
            };

            DX12Buffer tempBuffer(texelData,dataSize,Buffer::Accessibility::CPU_AND_GPU,D3D12_RESOURCE_STATE_COMMON,true);

            if(generateMips &&  mipLevels > 1)
            {
                updateMipMaps(onBuffer);
            }
            throw std::runtime_error("not implemented");
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

        D3D12_CPU_DESCRIPTOR_HANDLE DX12Texture::descriptorHandle()
        {
            return _view;
        }

    } // dx
} // slag