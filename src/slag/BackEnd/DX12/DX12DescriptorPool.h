#ifndef SLAG_DX12DESCRIPTORPOOL_H
#define SLAG_DX12DESCRIPTORPOOL_H

#include <d3d12.h>
#include "../../DescriptorPool.h"
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
        private:
            void move(DX12DescriptorPool&& from);
            ID3D12DescriptorHeap* _descriptorHeap = nullptr;
        };

    } // dx
} // slag

#endif //SLAG_DX12DESCRIPTORPOOL_H
