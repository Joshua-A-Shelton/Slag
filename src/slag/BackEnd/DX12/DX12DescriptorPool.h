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
            DX12DescriptorPool(size_t descriptorHeapSize, size_t textureHeapSize);
            ~DX12DescriptorPool()override;
            DX12DescriptorPool(const DX12DescriptorPool&)=delete;
            DX12DescriptorPool& operator=(const DX12DescriptorPool&)=delete;
            DX12DescriptorPool(DX12DescriptorPool&& from);
            DX12DescriptorPool& operator=(DX12DescriptorPool&& from);
            void reset()override;
        private:
            void move(DX12DescriptorPool&& from);
            ID3D12DescriptorHeap* _heaps[2]{nullptr, nullptr};
            ID3D12DescriptorHeap*& _descriptorHeap = _heaps[0];
            ID3D12DescriptorHeap*& _textureHeap = _heaps[1];
        };

    } // dx
} // slag

#endif //SLAG_DX12DESCRIPTORPOOL_H
