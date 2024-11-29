#include <stdexcept>
#include "DX12DescriptorPool.h"

namespace slag
{
    namespace dx
    {
        DX12DescriptorPool::DX12DescriptorPool(size_t descriptorHeapSize)
        {
            throw std::runtime_error("DX12DescriptorPool::DX12DescriptorPool not implemented");
        }

        DX12DescriptorPool::~DX12DescriptorPool()
        {

        }

        void DX12DescriptorPool::reset()
        {
            throw std::runtime_error("DX12DescriptorPool::reset not implemented");
        }

        void* makeBundleLowLevelHandle(DescriptorGroup* forGroup)
        {
            throw std::runtime_error("DX12DescriptorPool::makeBudleLowLevelHandle not implemented");
        }

    } // dx
} // slag