#include "common/Objects.h"

#include <cstring>

namespace slag {

void setDebugName(InstanceCommon* instance, ObjectRecord* record, const char* name)
{
    if (!name || !*name)
    {
        return;
    }
    size_t size = std::strlen(name) + 1;
    auto* copy = static_cast<char*>(instance->allocator.allocate(size, 1));
    if (copy)
    {
        std::memcpy(copy, name, size); // without memory the object stays unnamed
    }
    record->debugName = copy;
}

void releaseDebugName(InstanceCommon* instance, ObjectRecord* record)
{
    if (record->debugName)
    {
        instance->allocator.free(record->debugName, std::strlen(record->debugName) + 1);
        record->debugName = nullptr;
    }
}

} // namespace slag
