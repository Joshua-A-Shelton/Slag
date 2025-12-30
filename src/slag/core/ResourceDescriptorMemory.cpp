#include "ResourceDescriptorMemory.h"
#include <slag/backends/Backend.h>
namespace slag
{
    ResourceDescriptorMemory* ResourceDescriptorMemory::newResourceDescriptorMemory(uint64_t descriptorCount)
    {
        return Backend::current()->newResourceDescriptorMemory(descriptorCount);
    }
} // slag
