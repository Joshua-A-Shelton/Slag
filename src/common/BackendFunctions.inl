// Every backend entry point, in one list (ADR 0065). Each line expands to a
// function-pointer member of BackendFunctions and, in direct-dispatch builds,
// to a declaration in the backend's namespace.
//
// SLAG_BACKEND_FN(return type, name, (parameters))

SLAG_BACKEND_FN(Result<InstanceCommon*>, createInstance, (const InstanceDesc& desc, const HostAllocator& allocator))
SLAG_BACKEND_FN(void, destroyInstance, (InstanceCommon * instance))
