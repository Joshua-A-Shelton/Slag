#include "DescriptorPool.h"
#include "BackEnd/BackEndLib.h"
namespace slag
{
    DescriptorBundle DescriptorPool::makeBundle(DescriptorGroup* forGroup)
    {
        DescriptorBundle bundle;
        bundle._handle = makeBundleLowLevelHandle(forGroup);
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