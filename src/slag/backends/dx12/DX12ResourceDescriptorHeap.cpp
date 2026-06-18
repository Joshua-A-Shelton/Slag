#include "DX12ResourceDescriptorHeap.h"
#include <directx/d3dx12.h>
#include "DX12Backend.h"
#include "DX12Texture.h"
#include "DX12GraphicsCard.h"
#include "slag/exceptions/ResourceCreationError.h"
#include <slag/utilities/SLAG_ASSERT.h>

#include "DX12Buffer.h"
#include "slag/exceptions/NotImplemented.h"

namespace slag
{
    namespace dx12
    {
        DX12ResourceDescriptorHeap::DX12ResourceDescriptorHeap(DX12GraphicsCard* graphicsCard, uint32_t descriptorCount)
        {
            _graphicsCard = graphicsCard;
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = descriptorCount;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.NodeMask = 0;

            auto result = graphicsCard->device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));
            if (FAILED(result))
            {
                throw ResourceCreationError("Failed to create descriptor heap");
            }

            _descriptorSize = _graphicsCard->device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        DX12ResourceDescriptorHeap::DX12ResourceDescriptorHeap(DX12ResourceDescriptorHeap&& from) noexcept
        {
            move(from);
        }

        DX12ResourceDescriptorHeap& DX12ResourceDescriptorHeap::operator=(DX12ResourceDescriptorHeap&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12ResourceDescriptorHeap::~DX12ResourceDescriptorHeap()
        {
            if (_heap)
            {
                _heap->Release();
            }
        }

        GraphicsCard* DX12ResourceDescriptorHeap::graphicsCard()
        {
            return _graphicsCard;
        }

        uint32_t DX12ResourceDescriptorHeap::descriptorCount()
        {
            return _heap->GetDesc().NumDescriptors;
        }

        void DX12ResourceDescriptorHeap::setUniformTexture(uint32_t index, Texture* texture, uint32_t baseMip, uint32_t mipCount, uint32_t baseLayer, uint32_t layerCount)
        {
            auto dxTexture = static_cast<DX12Texture*>(texture);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_heap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(index, _descriptorSize);
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = DX12Backend::nativeFormat(dxTexture->format());
            srvDesc.ViewDimension = DX12Backend::nativeSRVTextureDimension(dxTexture->type(),dxTexture->layers(),dxTexture->sampleCount());
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            switch (srvDesc.ViewDimension)
            {
            case D3D12_SRV_DIMENSION_TEXTURE1D:
                srvDesc.Texture1D.MostDetailedMip = baseMip;
                srvDesc.Texture1D.MipLevels = mipCount;
                srvDesc.Texture1D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
                srvDesc.Texture1DArray.MostDetailedMip = baseMip;
                srvDesc.Texture1DArray.MipLevels = mipCount;
                srvDesc.Texture1DArray.ResourceMinLODClamp = 0;
                srvDesc.Texture1DArray.ArraySize = layerCount;
                srvDesc.Texture1DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2D:
                srvDesc.Texture2D.MostDetailedMip = baseMip;
                srvDesc.Texture2D.MipLevels = mipCount;
                srvDesc.Texture2D.ResourceMinLODClamp = 0;
                srvDesc.Texture2D.PlaneSlice = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
                srvDesc.Texture2DArray.MostDetailedMip = baseMip;
                srvDesc.Texture2DArray.MipLevels = mipCount;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                srvDesc.Texture2DArray.ArraySize = layerCount;
                srvDesc.Texture2DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMS:
                break;
            case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
                srvDesc.Texture2DMSArray.ArraySize = layerCount;
                srvDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_SRV_DIMENSION_TEXTURE3D:
                srvDesc.Texture3D.MostDetailedMip = baseMip;
                srvDesc.Texture3D.MipLevels = mipCount;
                srvDesc.Texture3D.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBE:
                srvDesc.TextureCube.MostDetailedMip = baseMip;
                srvDesc.TextureCube.MipLevels = mipCount;
                srvDesc.TextureCube.ResourceMinLODClamp = 0;
                break;
            case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
                srvDesc.TextureCubeArray.MostDetailedMip = baseMip;
                srvDesc.TextureCubeArray.MipLevels = mipCount;
                srvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                srvDesc.TextureCubeArray.NumCubes = layerCount/6;
                srvDesc.TextureCubeArray.First2DArrayFace = baseLayer;
                break;
            }
            _graphicsCard->device()->CreateShaderResourceView(dxTexture->dx12Handle(),&srvDesc,handle);
        }

        void DX12ResourceDescriptorHeap::setUnorderedAccessTexture(uint32_t index, Texture* texture, uint32_t mip, uint32_t baseLayer, uint32_t layerCount)
        {
            SLAG_ASSERT(texture->type() != TextureType::CUBE_MAP && "Unordered access textures cannot be cube maps");
            auto dxTexture = static_cast<DX12Texture*>(texture);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_heap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(index, _descriptorSize);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = DX12Backend::nativeFormat(dxTexture->format());
            uavDesc.ViewDimension = DX12Backend::nativeUAVTextureDimension(dxTexture->type(),dxTexture->layers(),dxTexture->sampleCount());
            switch (uavDesc.ViewDimension)
            {
            case D3D12_UAV_DIMENSION_TEXTURE1D:
                uavDesc.Texture1D.MipSlice = mip;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
                uavDesc.Texture1DArray.MipSlice = mip;
                uavDesc.Texture1DArray.ArraySize = layerCount;
                uavDesc.Texture1DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2D:
                uavDesc.Texture2D.MipSlice = mip;
                uavDesc.Texture2D.PlaneSlice = 0;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
                uavDesc.Texture2DArray.MipSlice = mip;
                uavDesc.Texture2DArray.ArraySize = layerCount;
                uavDesc.Texture2DArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DMS:
                break;
            case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
                uavDesc.Texture2DMSArray.ArraySize = layerCount;
                uavDesc.Texture2DMSArray.FirstArraySlice = baseLayer;
                break;
            case D3D12_UAV_DIMENSION_TEXTURE3D:
                uavDesc.Texture3D.MipSlice = mip;
                uavDesc.Texture3D.FirstWSlice = baseLayer;
                uavDesc.Texture3D.WSize = layerCount;
                break;
            }
            _graphicsCard->device()->CreateUnorderedAccessView(dxTexture->dx12Handle(),nullptr,&uavDesc,handle);
        }

        void DX12ResourceDescriptorHeap::setUniformStructuredBuffer(uint32_t index, Buffer* buffer, uint32_t offset, uint32_t length)
        {
            auto dxBuffer = static_cast<DX12Buffer*>(buffer);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_heap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(index, _descriptorSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = dxBuffer->deviceAddress() + offset;
            cbvDesc.SizeInBytes = length;

            _graphicsCard->device()->CreateConstantBufferView(&cbvDesc,handle);
        }

        void DX12ResourceDescriptorHeap::setStorageStructuredBuffer(uint32_t index, Buffer* buffer,
            uint64_t elementIndex, uint64_t elementCount, uint64_t elementStride)
        {
            auto dxBuffer = static_cast<DX12Buffer*>(buffer);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_heap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(index, _descriptorSize);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.FirstElement = elementIndex;
            uavDesc.Buffer.NumElements = elementCount;
            uavDesc.Buffer.StructureByteStride = elementStride;
            _graphicsCard->device()->CreateUnorderedAccessView(dxBuffer->dx12Handle(),nullptr,&uavDesc,handle);
        }

        void DX12ResourceDescriptorHeap::setUniformTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format,
            uint64_t offset, uint64_t length)
        {
            throw NotImplemented();
        }

        void DX12ResourceDescriptorHeap::setStorageTexelBuffer(uint32_t index, Buffer* buffer, PixelFormat format,
            uint64_t offset, uint64_t length)
        {
            throw NotImplemented();
        }

        ID3D12DescriptorHeap* DX12ResourceDescriptorHeap::dx12Handle() const
        {
            return _heap;
        }

        void DX12ResourceDescriptorHeap::move(DX12ResourceDescriptorHeap& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_heap,from._heap);
            _descriptorSize = from._descriptorSize;
        }
    } // dx12
} // slag