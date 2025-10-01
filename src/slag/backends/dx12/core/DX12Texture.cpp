#include "DX12Texture.h"

#include "DX12Buffer.h"
#include "DX12CommandBuffer.h"
#include "DX12GraphicsCard.h"
#include "DX12Semaphore.h"
#include "slag/backends/dx12/DX12Backend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12Texture::DX12Texture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
        {
            construct(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount);
        }

        DX12Texture::DX12Texture(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount,void* texelData, uint64_t texelDataLength, TextureBufferMapping* mappings, uint32_t mappingCount)
        {
            construct(texelFormat,type,usageFlags,width,height,depth,mipLevels,layers,sampleCount);
            DX12Buffer texData(texelData,texelDataLength,Buffer::Accessibility::CPU_AND_GPU,Buffer::UsageFlags::DATA_BUFFER);
            DX12Semaphore finished(0);
            DX12CommandBuffer commandBuffer(GPUQueue::QueueType::TRANSFER);
            commandBuffer.begin();
            commandBuffer.copyBufferToTexture(&texData,this,mappings,mappingCount);
            commandBuffer.end();

            CommandBuffer* bufferPtr = &commandBuffer;
            SemaphoreValue signal
            {
                .semaphore = &finished,
                .value = 1
            };
            QueueSubmissionBatch batch
            {
                .waitSemaphores = nullptr,
                .waitSemaphoreCount = 0,
                .commandBuffers = &bufferPtr,
                .commandBufferCount = 1,
                .signalSemaphores = &signal,
                .signalSemaphoreCount = 1
            };

            DX12GraphicsCard::selected()->transferQueue()->submit(&batch,1);
            finished.waitForValue(1);
        }

        DX12Texture::~DX12Texture()
        {
            if (_texture)
            {
                _texture->Release();
            }
            if (_allocation)
            {
                _allocation->Release();
            }
            if (_heap)
            {
                _heap->Release();
            }
        }

        Texture::Type DX12Texture::type()
        {
            return _type;
        }

        Texture::UsageFlags DX12Texture::usageFlags()
        {
            return _usage;
        }

        Texture::SampleCount DX12Texture::sampleCount()
        {
            return _sampleCount;
        }

        uint32_t DX12Texture::width()
        {
            return _width;
        }

        uint32_t DX12Texture::height()
        {
            return _height;
        }

        uint32_t DX12Texture::depth()
        {
            return _depth;
        }

        uint32_t DX12Texture::layers()
        {
            return _layers;
        }

        uint32_t DX12Texture::mipLevels()
        {
            return _mipLevels;
        }

        Pixels::Format DX12Texture::format()
        {
            return _format;
        }

        ID3D12Resource* DX12Texture::dx12Handle() const
        {
            return _texture;
        }

        void DX12Texture::construct(Pixels::Format texelFormat, Type type, UsageFlags usageFlags, uint32_t width,uint32_t height, uint32_t depth, uint32_t mipLevels, uint32_t layers, Texture::SampleCount sampleCount)
        {
            SLAG_ASSERT(((type != Type::TEXTURE_3D) || (type == Type::TEXTURE_3D && layers == 1)) && "3D textures must only have one layer");
            SLAG_ASSERT(((type == Type::TEXTURE_3D) || (type != Type::TEXTURE_3D && depth ==1)) && "Non 3D textures must only have a depth of 1");
            SLAG_ASSERT(((type !=Type::TEXTURE_1D) || (type == Texture::Type::TEXTURE_1D && height == 1)) && "1D textures must have a height of 1");
            SLAG_ASSERT((type != Type::TEXTURE_CUBE || (type == Texture::Type::TEXTURE_CUBE && layers == 6)) && "Cube Textures must have 6 layers");
            SLAG_ASSERT(width >= 1 && height >= 1 && depth >= 1 && mipLevels >=1 && layers >= 1 && "Width, height, depth, mipLevels and layers must be at least 1");
            _width = width;
            _height = height;
            _depth = depth;
            _mipLevels = mipLevels;
            _layers = layers;
            _format = texelFormat;
            _usage = usageFlags;
            _sampleCount = sampleCount;
            _type = type;

            uint16_t depthArray = std::max(depth, layers);

            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = DX12Backend::dx12Dimension(type);
            resourceDesc.Alignment = 0;
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.DepthOrArraySize = depthArray;
            resourceDesc.MipLevels = mipLevels;
            resourceDesc.Format = DX12Backend::dx12Format(texelFormat);
            resourceDesc.SampleDesc.Count = static_cast<UINT>(_sampleCount);
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = DX12Backend::dx12UsageFlags(usageFlags);

            auto allocInfo = DX12GraphicsCard::selected()->device()->GetResourceAllocationInfo(0,1,&resourceDesc);
            resourceDesc.Alignment = allocInfo.Alignment;

            D3D12MA::ALLOCATION_DESC allocationDesc = {};
            allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

            DX12GraphicsCard::selected()->allocator()->CreateResource(&allocationDesc,&resourceDesc,D3D12_RESOURCE_STATE_COMMON,nullptr,&_allocation, IID_PPV_ARGS(&_texture));

            D3D12_DESCRIPTOR_HEAP_DESC desc = {};
            desc.NumDescriptors = 1;
            if((uint8_t)(usageFlags & Texture::UsageFlags::RENDER_TARGET_ATTACHMENT))
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                DX12GraphicsCard::selected()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12GraphicsCard::selected()->device()->CreateRenderTargetView(_texture, nullptr,_view);
            }
            else if((uint8_t)(usageFlags & Texture::UsageFlags::DEPTH_STENCIL_ATTACHMENT))
            {
                desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
                DX12GraphicsCard::selected()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_heap));
                _view = _heap->GetCPUDescriptorHandleForHeapStart();
                DX12GraphicsCard::selected()->device()->CreateDepthStencilView(_texture, nullptr,_view);
            }
        }
    } // dx12
} // slag
