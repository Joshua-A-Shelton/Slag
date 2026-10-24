#include "common/Platform.h"
#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

Result<QueryPoolCommon*> createQueryPool(GraphicsDeviceCommon* common, const QueryPoolDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* pool = device.instance->allocator.create<QueryPoolImpl>();
    if (!pool)
    {
        return Status::OutOfHostMemory;
    }
    pool->device = &device;
    pool->functions = device.functions;
    // Copy queues have their own timestamp heap type; graphics and compute share one.
    D3D12_QUERY_HEAP_DESC heapDesc = {};
    heapDesc.Type = desc.queueType == CommandQueueType::Copy ? D3D12_QUERY_HEAP_TYPE_COPY_QUEUE_TIMESTAMP
                                                             : D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    heapDesc.Count = desc.count;
    HRESULT result = device.device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&pool->heap));
    if (FAILED(result))
    {
        device.instance->allocator.destroy(pool);
        return toStatus(device, result);
    }
    setObjectName(pool->heap, desc.debugName);
    return static_cast<QueryPoolCommon*>(pool);
}

void destroyQueryPool(GraphicsDeviceCommon* common, QueryPoolCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* pool = static_cast<QueryPoolImpl*>(object);
    release(pool->heap);
    destroyObject(device.instance, pool);
}

void cmdWriteTimestamp(CommandBufferCommon* object, QueryPoolCommon* poolObject, uint32_t index)
{
    static_cast<CommandBufferImpl*>(object)->list->EndQuery(static_cast<QueryPoolImpl*>(poolObject)->heap,
                                                            D3D12_QUERY_TYPE_TIMESTAMP, index);
}

void cmdResolveQueries(CommandBufferCommon* object, QueryPoolCommon* poolObject, uint32_t first, uint32_t count,
                       BufferCommon* buffer, uint64_t offset)
{
    static_cast<CommandBufferImpl*>(object)->list->ResolveQueryData(static_cast<QueryPoolImpl*>(poolObject)->heap,
                                                                    D3D12_QUERY_TYPE_TIMESTAMP, first, count,
                                                                    static_cast<BufferImpl*>(buffer)->resource, offset);
}

uint64_t timestampFrequency(const CommandQueueCommon* object)
{
    UINT64 frequency = 0;
    (void)static_cast<const CommandQueueImpl*>(object)->queue->GetTimestampFrequency(&frequency);
    return frequency;
}

Result<TimestampCalibration> calibrateTimestamps(const CommandQueueCommon* object)
{
    auto* queue = static_cast<const CommandQueueImpl*>(object);
    UINT64 gpu = 0;
    UINT64 cpu = 0; // QueryPerformanceCounter ticks
    HRESULT result = queue->queue->GetClockCalibration(&gpu, &cpu);
    if (FAILED(result))
    {
        return toStatus(*static_cast<GraphicsDeviceImpl*>(queue->device), result);
    }
    return TimestampCalibration{gpu, hostTicksToNanoseconds(cpu)};
}

} // namespace slag::d3d12
