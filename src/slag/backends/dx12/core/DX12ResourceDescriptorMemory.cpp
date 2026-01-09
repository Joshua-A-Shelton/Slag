#include "DX12ResourceDescriptorMemory.h"

#include "DX12Buffer.h"
#include "DX12GraphicsCard.h"

#include "DX12Texture.h"
#include "slag/backends/dx12/DX12Backend.h"
#include "slag/utilities/SLAG_ASSERT.h"

namespace slag
{
    namespace dx12
    {
        DX12ResourceDescriptorMemory::DX12ResourceDescriptorMemory(uint64_t descriptorCount)
        {
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = descriptorCount;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            DX12GraphicsCard::selected()->device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));
            _descriptorSize = DX12GraphicsCard::selected()->resourceDescriptorSize();
            _size = descriptorCount*_descriptorSize;
        }

        DX12ResourceDescriptorMemory::~DX12ResourceDescriptorMemory()
        {
            if (_heap)
            {
                _heap->Release();
            }
        }

        DX12ResourceDescriptorMemory::DX12ResourceDescriptorMemory(DX12ResourceDescriptorMemory&& from)
        {
            move(from);
        }

        DX12ResourceDescriptorMemory& DX12ResourceDescriptorMemory::operator=(DX12ResourceDescriptorMemory&& from)
        {
            move(from);
            return *this;
        }

        uint64_t DX12ResourceDescriptorMemory::nextDescriptorGroupOffset(uint64_t memoryOffset)
        {
            auto offAlign = memoryOffset % _descriptorSize;
            if (offAlign == 0)
            {
                return memoryOffset;
            }
            return memoryOffset + (_descriptorSize - (offAlign));
        }

        uint64_t DX12ResourceDescriptorMemory::size()
        {
            return _size;
        }

        uint64_t DX12ResourceDescriptorMemory::handle()
        {
            return  _heap->GetGPUDescriptorHandleForHeapStart().ptr;
        }

        void DX12ResourceDescriptorMemory::setSampledTexture(uint64_t memoryLocation, Texture* texture)
        {
            SLAG_ASSERT((texture->usageFlags() & Texture::UsageFlags::SAMPLED_IMAGE) == Texture::UsageFlags::SAMPLED_IMAGE && "Given texture is not a sampled texture");
            DX12Texture* dxTexture = static_cast<DX12Texture*>(texture);

            auto handle = _heap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += memoryLocation;
            DX12GraphicsCard::selected()->device()->CopyDescriptorsSimple(1,handle,dxTexture->shaderResourceViewHandle(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void DX12ResourceDescriptorMemory::setStorageTexture(uint64_t memoryLocation, Texture* texture)
        {
            SLAG_ASSERT((texture->usageFlags() & Texture::UsageFlags::STORAGE) == Texture::UsageFlags::STORAGE && "Given texture is not a storage texture");
            DX12Texture* dxTexture = static_cast<DX12Texture*>(texture);

            auto handle = _heap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += memoryLocation;
            DX12GraphicsCard::selected()->device()->CopyDescriptorsSimple(1,handle,dxTexture->unorderedAccessViewHandle(),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        void DX12ResourceDescriptorMemory::setUniformTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER) == Buffer::UsageFlags::UNIFORM_TEXEL_BUFFER && "Given Buffer is not a uniform texel buffer");
            SLAG_ASSERT(Pixels::AspectFlags(format) == Pixels::AspectFlags::COLOR && "Only color formats can be bound as texel buffer");
            auto size = Pixels::size(format);
            SLAG_ASSERT((startIndex + elementCount)*size <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            DX12Buffer* dxBuffer = static_cast<DX12Buffer*>(buffer);
            auto heapHandle = _heap->GetCPUDescriptorHandleForHeapStart();
            heapHandle.ptr += memoryLocation;

            D3D12_CPU_DESCRIPTOR_HANDLE handle;
            D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
            viewDesc.Format = DX12Backend::dx12Format(format);
            viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            viewDesc.Buffer.FirstElement = startIndex;
            viewDesc.Buffer.NumElements = elementCount;
            viewDesc.Buffer.StructureByteStride = 0;
            DX12GraphicsCard::selected()->device()->CreateShaderResourceView(dxBuffer->dx12Handle(), &viewDesc, heapHandle);

        }

        void DX12ResourceDescriptorMemory::setStorageTexelBuffer(uint64_t memoryLocation, Buffer* buffer, Pixels::Format format, uint64_t startIndex, uint64_t elementCount)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::STORAGE_TEXEL_BUFFER) == Buffer::UsageFlags::STORAGE_TEXEL_BUFFER && "Given Buffer is not a storage texel buffer");
            SLAG_ASSERT(Pixels::AspectFlags(format) == Pixels::AspectFlags::COLOR && "Only color formats can be bound as texel buffer");
            auto size = Pixels::size(format);
            SLAG_ASSERT((startIndex + elementCount)*size <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            DX12Buffer* dxBuffer = static_cast<DX12Buffer*>(buffer);
            auto heapHandle = _heap->GetCPUDescriptorHandleForHeapStart();
            heapHandle.ptr += memoryLocation;

            D3D12_CPU_DESCRIPTOR_HANDLE handle;
            D3D12_UNORDERED_ACCESS_VIEW_DESC  viewDesc = {};
            viewDesc.Format = DX12Backend::dx12Format(format);
            viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            viewDesc.Buffer.FirstElement = startIndex;
            viewDesc.Buffer.NumElements = elementCount;
            viewDesc.Buffer.StructureByteStride = 0;
            DX12GraphicsCard::selected()->device()->CreateUnorderedAccessView(dxBuffer->dx12Handle(), nullptr, &viewDesc, heapHandle);
        }

        void DX12ResourceDescriptorMemory::setUniformBuffer(uint64_t memoryLocation, Buffer* buffer, uint64_t offset, uint64_t length)
        {
            SLAG_ASSERT((buffer->usage() & Buffer::UsageFlags::UNIFORM_BUFFER) == Buffer::UsageFlags::UNIFORM_BUFFER && "Given Buffer is not a uniform buffer");
            SLAG_ASSERT(offset+length <= buffer->size() && "attempted to bind descriptor that exceeds buffer length");
            DX12Buffer* dxBuffer = static_cast<DX12Buffer*>(buffer);
            auto heapHandle = _heap->GetCPUDescriptorHandleForHeapStart();
            heapHandle.ptr += memoryLocation;

            auto handle = dxBuffer->gpuHandle();
            handle += offset;
            D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
            viewDesc.BufferLocation = handle;
            viewDesc.SizeInBytes = length;

            DX12GraphicsCard::selected()->device()->CreateConstantBufferView(&viewDesc, heapHandle);
        }

        void DX12ResourceDescriptorMemory::move(DX12ResourceDescriptorMemory& from)
        {
            std::swap(_heap, from._heap);
            _descriptorSize = from._descriptorSize;
            _size = from._size;
        }
    } // dx12
} // slag
