#include <cassert>
#include "DX12Texture.h"
#include "DX12Lib.h"
#include "DX12Buffer.h"
#include <directx/d3dx12.h>

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
                _disposeFunction = [=]
                {
                    dx12Texture->Release();
                    if(heap){heap->Release();}
                };
            }
            else
            {
                _disposeFunction = [=]
                {
                    if(heap)(heap->Release());
                };
            }
        }

        DX12Texture::DX12Texture(void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, Texture::Layout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            DX12CommandBuffer commandBuffer(GpuQueue::QueueType::Compute);
            commandBuffer.begin();
            build(&commandBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
            commandBuffer.end();
            DX12Lib::card()->computeQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        DX12Texture::DX12Texture(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, Texture::Layout initializedLayout, bool generateMips, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            build(onBuffer,texelData,dataSize,dataFormat,textureFormat,width,height,mipLevels,usage,initializedLayout,generateMips);
        }

        DX12Texture::~DX12Texture()
        {
            if(_allocation || _heap)
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

        void DX12Texture::build(DX12CommandBuffer* onBuffer, void* texelData, size_t dataSize, DXGI_FORMAT dataFormat, DXGI_FORMAT textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, Texture::Layout initializedLayout, bool generateMips)
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

            Texture::Layout currentLayout = Texture::Layout::UNDEFINED;
            if(texelData && dataSize)
            {
                DX12CommandBuffer commands(GpuQueue::QueueType::Transfer);
                commands.begin();
                if(dataFormat != textureFormat)
                {
                    DX12Texture tempTexture(texelData,dataSize,dataFormat,dataFormat,width,height,1,usage,Texture::Layout::SHADER_RESOURCE,false,true);

                    throw std::runtime_error("DX12Texture::build isn't finished implementing, unable to use different data format and texture format");

                    commands.end();
                    DX12Lib::card()->transferQueue()->submit(&commands);
                    commands.waitUntilFinished();

                    currentLayout = Texture::Layout::RENDER_TARGET;
                }
                else
                {
                    DX12Buffer tempBuffer(dataSize,slag::Buffer::Accessibility::CPU_AND_GPU,D3D12_RESOURCE_STATE_COMMON, true);
                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footPrint;
                    UINT numRows=0;
                    UINT64 rowSizeInBytes=0;
                    UINT64 totalBytes=0;
                    DX12Lib::card()->device()->GetCopyableFootprints(&resourceDesc,0,1,0,&footPrint,&numRows,&rowSizeInBytes,&totalBytes);

                    D3D12_SUBRESOURCE_DATA resourceData;
                    resourceData.pData=texelData;
                    resourceData.RowPitch = footPrint.Footprint.RowPitch;
                    resourceData.SlicePitch = resourceData.RowPitch* _height;

                    ImageBarrier imBarrier{};
                    imBarrier.texture = this;
                    imBarrier.oldLayout = Texture::Layout::UNDEFINED;
                    imBarrier.newLayout = Texture::Layout::TRANSFER_DESTINATION;
                    imBarrier.accessBefore = BarrierAccessFlags::NONE;
                    imBarrier.accessAfter = BarrierAccessFlags::TRANSFER_WRITE;
                    imBarrier.syncBefore = PipelineStageFlags::NONE;
                    imBarrier.syncAfter = PipelineStageFlags::ALL_COMMANDS;
                    commands.insertBarriers(&imBarrier,1, nullptr,0, nullptr,0);
                    UpdateSubresources(commands.underlyingCommandBuffer(),_texture,tempBuffer.underlyingBuffer(),0,0,1,&resourceData);

                    imBarrier.oldLayout = Texture::Layout::TRANSFER_DESTINATION;
                    //if we're not generating mip maps, put it in the final layout, otherwise, make it layout general
                    imBarrier.newLayout = generateMips? Texture::Layout::GENERAL : initializedLayout;
                    imBarrier.accessBefore = BarrierAccessFlags::TRANSFER_WRITE;
                    imBarrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
                    imBarrier.syncBefore = PipelineStageFlags::TRANSFER;
                    imBarrier.syncAfter = PipelineStageFlags::ALL_COMMANDS;
                    commands.insertBarriers(&imBarrier,1, nullptr,0, nullptr,0);

                    commands.end();
                    DX12Lib::card()->transferQueue()->submit(&commands);
                    commands.waitUntilFinished();

                    currentLayout = imBarrier.newLayout;
                }

            }

            if(generateMips)
            {
                updateMipMaps(onBuffer);
                ImageBarrier imBarrier{};
                imBarrier.texture = this;
                imBarrier.oldLayout = currentLayout;
                imBarrier.newLayout = initializedLayout;
                imBarrier.accessBefore = BarrierAccessFlags::NONE;
                imBarrier.accessAfter = BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE;
                imBarrier.syncBefore = PipelineStageFlags::ALL_COMMANDS;
                imBarrier.syncAfter = PipelineStageFlags::ALL_COMMANDS;
                onBuffer->insertBarriers(&imBarrier,1, nullptr,0, nullptr,0);
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
                    updateMipMapsGraphics(onBuffer);
                    break;
                case GpuQueue::Compute:
                    throw std::runtime_error("Generating mip maps on Compute Queue not implemented yet");
                    break;
                case GpuQueue::Transfer:
                    throw std::runtime_error("Cannot generate mip maps on Transfer Queue");
                    break;
            }
        }

        void DX12Texture::updateMipMapsGraphics(DX12CommandBuffer* onBuffer)
        {
            throw std::runtime_error("DX12Texture::updateMipMapsGraphics not implemented");
        }


        D3D12_CPU_DESCRIPTOR_HANDLE DX12Texture::descriptorHandle()
        {
            return _view;
        }

    } // dx
} // slag