#include <cassert>
#include "DX12Texture.h"
#include "DX12Lib.h"
#include "DX12Buffer.h"
#include <directx/d3dx12.h>

namespace slag
{
    namespace dx
    {
        DX12Texture::DX12Texture(ID3D12Resource* dx12Texture, bool ownTexture, Pixels::Format textureFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately): resources::Resource(destroyImmediately)
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

        DX12Texture::DX12Texture(void** texelDataArray, size_t texelDataCount, uint64_t dataSize, Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS usage, Texture::Layout initializedLayout, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            construct(dataFormat,type,width,height,texelDataCount,mipLevels,1,usage,Texture::Layout::TRANSFER_DESTINATION);
            DX12CommandBuffer commandBuffer(GpuQueue::TRANSFER);
            commandBuffer.begin();

            std::vector<DX12Buffer> dataBuffers;
            for(int i=0; i<texelDataCount; i++)
            {
                dataBuffers.emplace_back(texelDataArray[i],dataSize,Buffer::Accessibility::CPU_AND_GPU,D3D12_RESOURCE_STATE_COPY_SOURCE,true);
                commandBuffer.copyBufferToImage(&dataBuffers[i],0,this,Texture::Layout::TRANSFER_DESTINATION,0,0);
            }
            ImageBarrier imageBarrier
            {
                    .texture=this,
                    .oldLayout=Texture::Layout::TRANSFER_DESTINATION,
                    .newLayout=initializedLayout,
                    .accessBefore = BarrierAccessFlags::TRANSFER_WRITE,
                    .accessAfter=BarrierAccessFlags::ALL_READ | BarrierAccessFlags::ALL_WRITE,
                    .syncBefore=PipelineStageFlags::TRANSFER,
                    .syncAfter =PipelineStageFlags::ALL_COMMANDS
            };

            commandBuffer.insertBarriers(&imageBarrier,1, nullptr,0, nullptr,0);

            commandBuffer.end();
            DX12Lib::card()->transferQueue()->submit(&commandBuffer);
            commandBuffer.waitUntilFinished();
        }

        DX12Texture::DX12Texture(Pixels::Format dataFormat, Texture::Type type, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, uint8_t sampleCount, D3D12_RESOURCE_FLAGS usage, bool destroyImmediately): resources::Resource(destroyImmediately)
        {
            construct(dataFormat,type,width,height,layers,mipLevels,sampleCount,usage,Texture::Layout::UNDEFINED);
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
            _type = from._type;
            _width = from._width;
            _height = from._height;
            _mipLevels = from._mipLevels;
            _layers = from._layers;
            _sampleCount = from._sampleCount;
            _format = from._format;
            _usage = from._usage;
        }

        Texture::Type DX12Texture::type()
        {
            return _type;
        }

        uint32_t DX12Texture::width()
        {
            return _width;
        }

        uint32_t DX12Texture::height()
        {
            return _height;
        }

        uint32_t DX12Texture::layers()
        {
            return _layers;
        }

        uint32_t DX12Texture::mipLevels()
        {
            return _mipLevels;
        }

        uint8_t DX12Texture::sampleCount()
        {
            return _sampleCount;
        }

        Pixels::Format DX12Texture::format()
        {
            return _format;
        }

        ID3D12Resource* DX12Texture::texture()
        {
            return _texture;
        }


        D3D12_CPU_DESCRIPTOR_HANDLE DX12Texture::descriptorHandle()
        {
            return _view;
        }

        void DX12Texture::construct(Pixels::Format dataFormat, Texture::Type textureType, uint32_t width, uint32_t height,uint32_t layers, uint32_t mipLevels, uint8_t samples, D3D12_RESOURCE_FLAGS usage, Texture::Layout initialLayout)
        {
            assert(!(usage & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET && usage & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) && "Texture cannot be both render target and depth stencil");
            _width = width;
            _height = height;
            _mipLevels = mipLevels;
            _layers = layers;
            _format = dataFormat;
            _usage = usage;
            _sampleCount = samples;
            _type = textureType;

            auto localFormat = DX12Lib::format(dataFormat);

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = DX12Lib::dimension(textureType);
            resourceDesc.Alignment = 0;
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.DepthOrArraySize = layers;
            resourceDesc.MipLevels = mipLevels;
            resourceDesc.Format = localFormat;
            resourceDesc.SampleDesc.Count = samples;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = usage;

            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            DX12Lib::card()->allocator()->CreateResource(&allocationDesc,&resourceDesc,DX12Lib::stateLayout(initialLayout),nullptr,&_allocation, IID_PPV_ARGS(&_texture));

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
        }

        DXGI_FORMAT DX12Texture::underlyingFormat()
        {
            return DX12Lib::format(_format);
        }

        D3D12_RESOURCE_FLAGS DX12Texture::usage()const
        {
            return _usage;
        }

    } // dx
} // slag