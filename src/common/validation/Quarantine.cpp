#include "common/validation/Quarantine.h"

namespace slag {

void Quarantine::add(InstanceCommon* instance, void* object, ReleaseFn release)
{
    Entry oldest;
    {
        std::lock_guard<std::mutex> guard(lock);
        oldest = entries[next];
        entries[next] = {object, release};
        next = (next + 1) % capacity;
    }
    if (oldest.object)
    {
        oldest.release(instance, oldest.object);
    }
}

void Quarantine::flush(InstanceCommon* instance)
{
    std::lock_guard<std::mutex> guard(lock);
    for (Entry& entry : entries)
    {
        if (entry.object)
        {
            entry.release(instance, entry.object);
            entry = {};
        }
    }
    next = 0;
}

} // namespace slag
