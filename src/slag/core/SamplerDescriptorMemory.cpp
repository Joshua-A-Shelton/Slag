#include "SamplerDescriptorMemory.h"
#include <slag/backends/Backend.h>
namespace slag
{
    DescriptorMemory::Type SamplerDescriptorMemory::type()
    {
        return DescriptorMemory::Type::SAMPLER;
    }

    SamplerDescriptorMemory* SamplerDescriptorMemory::newSamplerDescriptorMemory(uint64_t descriptorCount)
    {
        return Backend::current()->newSamplerDescriptorMemory(descriptorCount);
    }
} // slag
