#include "DescriptorPool.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    DescriptorBundle DescriptorPool::makeBundle(DescriptorGroup* forGroup)
    {
        DescriptorBundle bundle;
        setBundleLowLevelHandles(&bundle._gpuhandle, &bundle._cpuhandle, forGroup);
        return bundle;
    }

    DescriptorPool* DescriptorPool::newDescriptorPool()
    {
        return lib::BackEndLib::get()->newDescriptorPool(DescriptorPoolPageInfo{});
    }

    DescriptorPool* DescriptorPool::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
    {
        return lib::BackEndLib::get()->newDescriptorPool(pageInfo);
    }
} // slag