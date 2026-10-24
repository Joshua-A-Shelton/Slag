#pragma once

// Data every backend shares, between the public classes and the backend
// types (ADR 0065).

#include "common/Backend.h"
#include "common/HostAllocator.h"
#include "common/Vector.h"

#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>

#include <cstdint>

namespace slag {

struct InstanceCommon : Instance
{
    const BackendInfo* backend = nullptr;
    const BackendFunctions* functions = nullptr;
    HostAllocator allocator;
    DebugMessageFn debugCallback = nullptr;
    void* debugUserData = nullptr;
    ValidationFlags validation = ValidationFlags::None;
    ValidationFailureAction failureAction = ValidationFailureAction::Break;
    Vector<GraphicsDevice*> devices;
    uint64_t userData = 0;
};

// Fills the shared fields; backends call this first when creating an instance.
void initInstanceCommon(InstanceCommon& instance, const InstanceDesc& desc, const BackendInfo& backend,
                        const HostAllocator& allocator);

} // namespace slag
