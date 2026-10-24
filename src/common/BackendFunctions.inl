// Every backend entry point, in one list (ADR 0065). Each line expands to a
// function-pointer member of BackendFunctions and, in direct-dispatch builds,
// to a declaration in the backend's namespace.
//
// SLAG_BACKEND_FN(return type, name, (parameters))

SLAG_BACKEND_FN(Result<InstanceCommon*>, createInstance, (const InstanceDesc& desc, const HostAllocator& allocator))
SLAG_BACKEND_FN(void, destroyInstance, (InstanceCommon * instance))

SLAG_BACKEND_FN(Result<void>, initializeDevice, (GraphicsDeviceCommon * device, const GraphicsDeviceInitDesc& desc))
SLAG_BACKEND_FN(void, shutdownDevice, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(MemoryBudget, memoryBudget, (const GraphicsDeviceCommon* device))
SLAG_BACKEND_FN(Result<void>, deviceWaitIdle, (GraphicsDeviceCommon * device))
SLAG_BACKEND_FN(Result<void>, queueWaitIdle, (CommandQueueCommon * queue))

SLAG_BACKEND_FN(Result<SemaphoreCommon*>, createSemaphore, (GraphicsDeviceCommon * device, const SemaphoreDesc& desc))
SLAG_BACKEND_FN(void, destroySemaphore, (GraphicsDeviceCommon * device, SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(uint64_t, semaphoreValue, (const SemaphoreCommon* semaphore))
SLAG_BACKEND_FN(Result<void>, signalSemaphore, (SemaphoreCommon * semaphore, uint64_t value))
SLAG_BACKEND_FN(Result<void>, waitSemaphores,
                (GraphicsDeviceCommon * device, std::span<const SemaphoreValue> values, WaitMode mode,
                 uint64_t timeoutNs))

SLAG_BACKEND_FN(Result<BufferCommon*>, createBuffer, (GraphicsDeviceCommon * device, const BufferDesc& desc))
SLAG_BACKEND_FN(void, destroyBuffer, (GraphicsDeviceCommon * device, BufferCommon* buffer))

SLAG_BACKEND_FN(Result<PipelineCommon*>, createComputePipeline,
                (GraphicsDeviceCommon * device, const ComputePipelineDesc& desc))
SLAG_BACKEND_FN(void, destroyPipeline, (GraphicsDeviceCommon * device, PipelineCommon* pipeline))

SLAG_BACKEND_FN(void, writeRWStructuredBuffer,
                (GraphicsDeviceCommon * device, uint32_t index, const StructuredBufferDescriptor& descriptor))

SLAG_BACKEND_FN(Result<CommandBufferCommon*>, createCommandBuffer,
                (GraphicsDeviceCommon * device, const CommandBufferDesc& desc))
SLAG_BACKEND_FN(void, destroyCommandBuffer, (GraphicsDeviceCommon * device, CommandBufferCommon* commandBuffer))
SLAG_BACKEND_FN(Result<void>, beginCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(Result<void>, endCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(Result<void>, resetCommandBuffer, (CommandBufferCommon * commandBuffer))
SLAG_BACKEND_FN(void, cmdBindPipeline, (CommandBufferCommon * commandBuffer, PipelineCommon* pipeline))
SLAG_BACKEND_FN(void, cmdPushConstants,
                (CommandBufferCommon * commandBuffer, const void* data, uint32_t size, uint32_t offset))
SLAG_BACKEND_FN(void, cmdDispatch, (CommandBufferCommon * commandBuffer, uint32_t x, uint32_t y, uint32_t z))
SLAG_BACKEND_FN(void, cmdBarrier,
                (CommandBufferCommon * commandBuffer, std::span<const GlobalBarrier> globalBarriers,
                 std::span<const BufferBarrier> bufferBarriers, std::span<const TextureBarrier> textureBarriers))
SLAG_BACKEND_FN(void, cmdCopyBuffer,
                (CommandBufferCommon * commandBuffer, BufferCommon* destination, uint64_t destinationOffset,
                 BufferCommon* source, uint64_t sourceOffset, uint64_t size))

SLAG_BACKEND_FN(Result<void>, submit, (CommandQueueCommon * queue, const SubmitDesc& desc))
