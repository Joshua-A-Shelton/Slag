#include "SamplerDescriptorMemory.h"
#include <slag/backends/Backend.h>
namespace slag
{
    SamplerDescriptorMemory* SamplerDescriptorMemory::newSamplerDescriptorMemory(uint64_t descriptorCount)
    {
        return Backend::current()->newSamplerDescriptorMemory(descriptorCount);
    }
} // slag
