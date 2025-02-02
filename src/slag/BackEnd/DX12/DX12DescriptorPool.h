#ifndef SLAG_DX12DESCRIPTORPOOL_H
#define SLAG_DX12DESCRIPTORPOOL_H

#include "../../DescriptorPool.h"
#include <directx/d3d12.h>
#include <d3d12.h>

namespace slag
{
    namespace dx
    {

        class DX12DescriptorPool: public DescriptorPool
        {
        public:
            DX12DescriptorPool(size_t descriptorHeapSize);
            ~DX12DescriptorPool()override;
            DX12DescriptorPool(const DX12DescriptorPool&)=delete;
            DX12DescriptorPool& operator=(const DX12DescriptorPool&)=delete;
            DX12DescriptorPool(DX12DescriptorPool&& from);
            DX12DescriptorPool& operator=(DX12DescriptorPool&& from);
            void reset()override;
            void setBundleLowLevelHandles(void** gpuHandle, void** cpuHandle, DescriptorGroup* forGroup) override;
            ID3D12DescriptorHeap* underlyingHeap();
        private:
            void move(DX12DescriptorPool&& from);
            ID3D12DescriptorHeap* _descriptorHeap = nullptr;
            size_t _offset = 0;
            size_t _descriptorSize = 0;
        };

    } // dx
} // slag

#endif //SLAG_DX12DESCRIPTORPOOL_H
