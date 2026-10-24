#include "common/Platform.h"
#include "vulkan/VkBackend.h"

#include <cmath>

namespace slag::vk {

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
    VkQueryPoolCreateInfo createInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = desc.count;
    VkResult result = device.fn.vkCreateQueryPool(device.device, &createInfo, nullptr, &pool->pool);
    if (result != VK_SUCCESS)
    {
        device.instance->allocator.destroy(pool);
        return toStatus(device, result);
    }
    // Vulkan queries start unavailable and must be reset before their first
    // write; D3D12 has no reset, so every query starts reset (ADR 0051).
    device.fn.vkResetQueryPool(device.device, pool->pool, 0, desc.count);
    setObjectName(device, VK_OBJECT_TYPE_QUERY_POOL, reinterpret_cast<uint64_t>(pool->pool), desc.debugName);
    return static_cast<QueryPoolCommon*>(pool);
}

void destroyQueryPool(GraphicsDeviceCommon* common, QueryPoolCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* pool = static_cast<QueryPoolImpl*>(object);
    device.fn.vkDestroyQueryPool(device.device, pool->pool, nullptr);
    destroyObject(device.instance, pool);
}

void cmdWriteTimestamp(CommandBufferCommon* object, QueryPoolCommon* poolObject, uint32_t index)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
    // ALL_COMMANDS matches D3D12's EndQuery: the time once earlier work is done.
    device.fn.vkCmdWriteTimestamp2(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                                   static_cast<QueryPoolImpl*>(poolObject)->pool, index);
}

void cmdResolveQueries(CommandBufferCommon* object, QueryPoolCommon* poolObject, uint32_t first, uint32_t count,
                       BufferCommon* buffer, uint64_t offset)
{
    auto* commandBuffer = static_cast<CommandBufferImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(commandBuffer->device);
    VkQueryPool pool = static_cast<QueryPoolImpl*>(poolObject)->pool;
    // A copy (transfer stage, transfer write, like D3D12's ResolveQueryData),
    // then the reset that lets the queries be written again. Query commands on
    // the same queries execute in submission order, so the reset needs no barrier.
    device.fn.vkCmdCopyQueryPoolResults(commandBuffer->commandBuffer, pool, first, count,
                                        static_cast<BufferImpl*>(buffer)->buffer, offset, sizeof(uint64_t),
                                        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    device.fn.vkCmdResetQueryPool(commandBuffer->commandBuffer, pool, first, count);
}

uint64_t timestampFrequency(const CommandQueueCommon* object)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(object->device);
    // timestampPeriod is nanoseconds per tick, and not always whole (52.083 on some Intel GPUs).
    return static_cast<uint64_t>(std::llround(1e9 / static_cast<double>(device.properties.limits.timestampPeriod)));
}

Result<TimestampCalibration> calibrateTimestamps(const CommandQueueCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(object->device);
    VkCalibratedTimestampInfoKHR infos[2] = {{VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR},
                                             {VK_STRUCTURE_TYPE_CALIBRATED_TIMESTAMP_INFO_KHR}};
    infos[0].timeDomain = VK_TIME_DOMAIN_DEVICE_KHR;
    infos[1].timeDomain = hostTimeDomain;
    uint64_t timestamps[2] = {};
    uint64_t maxDeviation = 0;
    VkResult result = device.fn.vkGetCalibratedTimestampsKHR(device.device, 2, infos, timestamps, &maxDeviation);
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }
    return TimestampCalibration{timestamps[0], hostTicksToNanoseconds(timestamps[1])};
}

} // namespace slag::vk
