// Every backend entry point, in one list (ADR 0065). Each line expands to a
// function-pointer member of BackendFunctions and, in direct-dispatch builds,
// to a declaration in the backend's namespace.
//
// SLAG_BACKEND_FN(return type, name, (parameters))

SLAG_BACKEND_FN(Result<InstanceCommon*>, createInstance, (const InstanceDesc& desc, const HostAllocator& allocator))
SLAG_BACKEND_FN(void, destroyInstance, (InstanceCommon * instance))

SLAG_BACKEND_FN(Result<void>, initializeDevice, (GraphicsDeviceCommon * device, const GraphicsDeviceInitDesc& desc))
SLAG_BACKEND_FN(void, shutdownDevice, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(Result<void>, deviceWaitIdle, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(Result<void>, queueWaitIdle, (CommandQueueCommon * queue))

SLAG_BACKEND_FN(Result<SemaphoreCommon*>, createSemaphore, (GraphicsDeviceCommon * device, const SemaphoreDesc& desc))
SLAG_BACKEND_FN(void, destroySemaphore, (GraphicsDeviceCommon * device, SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(uint64_t, semaphoreValue, (const SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(Result<void>, signalSemaphore, (SemaphoreCommon * semaphore, uint64_t value))
SLAG_BACKEND_FN(Result<void>, waitSemaphores,
                (GraphicsDeviceCommon * device, std::span<const SemaphoreValue> values, WaitMode mode,
                 uint64_t timeoutNs))
