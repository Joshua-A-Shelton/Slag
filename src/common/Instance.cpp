#include "common/Backend.h"
#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/Instance.h>

namespace slag {

namespace {

const BackendInfo* findBackend(Backend backend)
{
    for (const BackendInfo* info : registeredBackends())
    {
        if (info->backend == backend)
        {
            return info;
        }
    }
    return nullptr;
}

} // namespace

bool isBackendAvailable(Backend backend)
{
    return findBackend(backend) != nullptr;
}

Result<Instance*> createInstance(const InstanceDesc& desc)
{
    MessageSink sink = messageSink(desc);

#ifdef SLAG_VALIDATION
    if ((desc.allocatorCallbacks.allocate == nullptr) != (desc.allocatorCallbacks.free == nullptr))
    {
        logMessage(
            sink, MessageSeverity::Error, MessageSource::Slag,
            "slag: createInstance: AllocatorCallbacks needs both allocate and free; using the default allocator");
    }
#endif

    const BackendInfo* backend = findBackend(desc.backend);
    if (!backend)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: createInstance: backend %u is not compiled into this build",
                   static_cast<unsigned>(desc.backend));
        return Status::Unsupported;
    }

    Result<InstanceCommon*> instance = backend->functions->createInstance(desc, HostAllocator(desc.allocatorCallbacks));
    if (!instance)
    {
        return instance.status();
    }
    return static_cast<Instance*>(instance.value());
}

void destroy(Instance* instance)
{
    if (!instance)
    {
        return;
    }
    auto* self = static_cast<InstanceCommon*>(instance);
#ifdef SLAG_VALIDATION
    self->quarantine.flush(self);
#endif
    SLAG_DISPATCH(self, destroyInstance)(self);
}

std::span<GraphicsDevice* const> Instance::graphicsDevices() const
{
    auto* self = static_cast<const InstanceCommon*>(this);
    return {self->devices.data(), self->devices.size()};
}

uint64_t Instance::userData() const
{
    return static_cast<const InstanceCommon*>(this)->userData;
}

void Instance::setUserData(uint64_t value)
{
    static_cast<InstanceCommon*>(this)->userData = value;
}

void initInstanceCommon(InstanceCommon& instance, const InstanceDesc& desc, const BackendInfo& backend,
                        const HostAllocator& allocator)
{
    instance.backend = &backend;
    instance.functions = backend.functions;
    instance.allocator = allocator;
    instance.messageCallback = desc.messageCallback;
    instance.messageUserData = desc.messageUserData;
    instance.validationFlags = desc.validationFlags;
    instance.failureAction = desc.validationFailureAction;
    instance.crashDiagnostics = desc.crashDiagnostics;
    instance.devices = Vector<GraphicsDevice*>(&instance.allocator);
    instance.userData = desc.userData;
}

} // namespace slag
