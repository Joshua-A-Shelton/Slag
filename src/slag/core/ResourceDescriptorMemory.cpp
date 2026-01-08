#include "ResourceDescriptorMemory.h"

#include "slag/backends/Backend.h"

namespace slag {
    DescriptorMemory::Type ResourceDescriptorMemory::type()
    {
        return DescriptorMemory::Type::RESOURCE;
    }

    ResourceDescriptorMemory* ResourceDescriptorMemory::newResourceDescriptorMemory(uint64_t descriptorCount)
    {
        return Backend::current()->newResourceDescriptorMemory(descriptorCount);
    }
} // slag