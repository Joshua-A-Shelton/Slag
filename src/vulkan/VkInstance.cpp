#include "vulkan/VkBackend.h"

namespace slag::vk {

// Implemented in step 2 of M1 (loader, instance, device listing).
Result<InstanceCommon*> createInstance(const InstanceDesc&, const HostAllocator&)
{
    return Status::Unsupported;
}

void destroyInstance(InstanceCommon*) {}

} // namespace slag::vk
