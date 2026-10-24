#pragma once

// QueryPool and QueryPoolDesc (ADR 0051).

#include <slag/CommandQueue.h>

#include <cstdint>

namespace slag {

// Timestamps: writeTimestamp. Occlusion: beginQuery / endQuery around draws
// in one render pass on a graphics queue, counting the samples that pass the
// depth and stencil tests exactly (ADR 0051).
enum class QueryType : uint8_t
{
    Timestamp,
    Occlusion,
};

// Every query is reset at creation; a query is resolved before it is written
// again (ADR 0051).
struct QueryPoolDesc
{
    QueryType type = QueryType::Timestamp;
    uint32_t count = 0;
    // Timestamps: the queue type the pool is written and resolved on;
    // occlusion pools are Graphics. Graphics
    // and compute pools work on either; D3D12 keeps copy-queue timestamps in
    // their own heap type, so copy pools only work on copy queues. Copy needs
    // GraphicsDeviceCapabilities::copyQueueTimestamps.
    CommandQueueType queueType = CommandQueueType::Graphics;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class QueryPool
{
public:
    uint64_t userData() const;
    void setUserData(uint64_t value);

    QueryPool(const QueryPool&) = delete;
    QueryPool& operator=(const QueryPool&) = delete;

protected:
    QueryPool() = default;
    ~QueryPool() = default;
};

} // namespace slag
