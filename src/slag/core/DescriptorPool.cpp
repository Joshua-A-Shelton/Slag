#include "DescriptorPool.h"
#include <slag/backends/Backend.h>
namespace slag
{
    DescriptorBundle DescriptorPool::makeBundle(DescriptorGroup* forGroup)
    {
        DescriptorBundle bundle;
        this->setBundleLowLevelHandles(&bundle._gpuhandle,&bundle._cpuhandle,forGroup);
        return bundle;
    }

    DescriptorPool* DescriptorPool::newDescriptorPool()
    {
        return Backend::current()->newDescriptorPool();
    }

    DescriptorPool* DescriptorPool::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
    {
        return Backend::current()->newDescriptorPool(pageInfo);
    }
} // slag
