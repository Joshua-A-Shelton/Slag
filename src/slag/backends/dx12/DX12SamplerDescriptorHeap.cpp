#include "DX12SamplerDescriptorHeap.h"
#include "DX12GraphicsCard.h"
#include <directx/d3dx12.h>

#include "DX12Sampler.h"
#include "slag/exceptions/ResourceCreationError.h"

namespace slag
{
    namespace dx12
    {
        DX12SamplerDescriptorHeap::DX12SamplerDescriptorHeap(DX12GraphicsCard* graphicsCard, uint32_t descriptorCount)
        {
            _graphicsCard = graphicsCard;
            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
            heapDesc.NumDescriptors = descriptorCount;
            heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            heapDesc.NodeMask = 0;

            auto result = graphicsCard->device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap));
            if (FAILED(result))
            {
                throw ResourceCreationError("Failed to create descriptor heap");
            }

        }

        DX12SamplerDescriptorHeap::DX12SamplerDescriptorHeap(DX12SamplerDescriptorHeap&& from) noexcept
        {
            move(from);
        }

        DX12SamplerDescriptorHeap& DX12SamplerDescriptorHeap::operator=(DX12SamplerDescriptorHeap&& from) noexcept
        {
            move(from);
            return *this;
        }

        DX12SamplerDescriptorHeap::~DX12SamplerDescriptorHeap()
        {
            if (_heap)
            {
                _heap->Release();
            }
        }

        void DX12SamplerDescriptorHeap::setSampler(uint64_t heapOffset, Sampler* sampler)
        {
            auto dxSampler = static_cast<DX12Sampler*>(sampler);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(_heap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapOffset, 1);

            _graphicsCard->device()->CreateSampler(&dxSampler->dx12Desc(),handle);
        }

        GraphicsCard* DX12SamplerDescriptorHeap::graphicsCard()
        {
            return _graphicsCard;
        }

        uint64_t DX12SamplerDescriptorHeap::size()
        {
            return _heap->GetDesc().NumDescriptors * _graphicsCard->descriptorHeapDetails().samplerDescriptorSize;
        }

        void* DX12SamplerDescriptorHeap::pointer()
        {
            return reinterpret_cast<void*>(_heap->GetCPUDescriptorHandleForHeapStart().ptr);
        }

        uint64_t DX12SamplerDescriptorHeap::deviceAddress()
        {
            return _heap->GetGPUDescriptorHandleForHeapStart().ptr;
        }

        ID3D12DescriptorHeap* DX12SamplerDescriptorHeap::dx12Handle() const
        {
            return _heap;
        }

        void DX12SamplerDescriptorHeap::move(DX12SamplerDescriptorHeap& from)
        {
            std::swap(_graphicsCard,from._graphicsCard);
            std::swap(_heap,from._heap);
        }
    } // dx12
} // slag