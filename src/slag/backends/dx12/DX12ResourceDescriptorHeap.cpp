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

        uint64_t DX12ResourceDescriptorHeap::size()
        {
            return _heap->GetDesc().NumDescriptors * _graphicsCard->descriptorHeapDetails().textureDescriptorSize;
        }

        void* DX12ResourceDescriptorHeap::data()
        {
            return reinterpret_cast<void*>(_heap->GetCPUDescriptorHandleForHeapStart().ptr);
        }

        uint64_t DX12ResourceDescriptorHeap::deviceAddress()
        {
            return _heap->GetGPUDescriptorHandleForHeapStart().ptr;
        }

        ID3D12DescriptorHeap* DX12ResourceDescriptorHeap::dx12Handle() const
        {
            return _heap;
        }

        void DX12ResourceDescriptorHeap::move(DX12ResourceDescriptorHeap& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_heap,from._heap);
        }
    } // dx12
} // slag