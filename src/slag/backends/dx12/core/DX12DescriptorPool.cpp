#include "DX12DescriptorPool.h"

#include "DX12GraphicsCard.h"

namespace slag
{
    namespace dx12
    {
        DX12DescriptorPool::DX12DescriptorPool(size_t descriptorHeapSize)
        {
            D3D12_DESCRIPTOR_HEAP_DESC desc{};
            desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            desc.NumDescriptors = descriptorHeapSize;

            DX12GraphicsCard::selected()->device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_descriptorHeap));
            _descriptorSize = DX12GraphicsCard::selected()->device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        DX12DescriptorPool::~DX12DescriptorPool()
        {
            if (_descriptorHeap != nullptr)
            {
                _descriptorHeap->Release();
            }
        }

        DX12DescriptorPool::DX12DescriptorPool(DX12DescriptorPool&& from)
        {
            move(from);
        }

        DX12DescriptorPool& DX12DescriptorPool::operator=(DX12DescriptorPool&& from)
        {
            move(from);
            return *this;
        }

        void DX12DescriptorPool::reset()
        {
            _offset = 0;
        }

        void DX12DescriptorPool::setBundleLowLevelHandles(void** gpuHandle, void** cpuHandle, DescriptorGroup* forGroup)
        {
            auto startGpu = _descriptorHeap->GetGPUDescriptorHandleForHeapStart();
            auto startCpu = _descriptorHeap->GetCPUDescriptorHandleForHeapStart();
            auto off = _offset*_descriptorSize;
            startGpu.ptr += off;
            startCpu.ptr += off;
            _offset+=forGroup->descriptorCount();
            *gpuHandle = std::bit_cast<void*>(startGpu);
            *cpuHandle = std::bit_cast<void*>(startCpu);
        }

        ID3D12DescriptorHeap* DX12DescriptorPool::dx12Handle()
        {
            return _descriptorHeap;
        }

        void DX12DescriptorPool::move(DX12DescriptorPool& from)
        {
            std::swap(_descriptorHeap,from._descriptorHeap);
            _offset = from._offset;
            _descriptorSize = from._descriptorSize;
        }
    } // dx12
} // slag
