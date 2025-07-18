#include "DescriptorPool.h"
#include <slag/backends/Backend.h>
namespace slag
{
    DescriptorPool* DescriptorPool::newDescriptorPool()
    {
        return Backend::current()->newDescriptorPool();
    }

    DescriptorPool* DescriptorPool::newDescriptorPool(const DescriptorPoolPageInfo& pageInfo)
    {
        return Backend::current()->newDescriptorPool(pageInfo);
    }
} // slag
