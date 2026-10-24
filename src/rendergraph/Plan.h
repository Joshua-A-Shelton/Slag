#pragma once

// The render graph's planner (ADR 0072): from passes and their resource uses,
// in declaration order, works out the submits ("batches") and the semaphore
// waits between queues, the barriers at the start and end of every pass, and
// where transient resources sit in shared memory. Plain data in and out, so
// it is tested without a GPU.

#include "common/HostAllocator.h"
#include "common/Vector.h"

#include <slag/Barriers.h>
#include <slag/CommandQueue.h>
#include <slag/Result.h>

#include <cstdint>
#include <span>

namespace slag::rg {

constexpr uint32_t none = UINT32_MAX;

struct PlanResource
{
    bool texture = true;
    bool transient = false;
    TextureLayout initialLayout = TextureLayout::Undefined; // imported textures
    TextureLayout finalLayout = TextureLayout::Common;      // imported textures
    uint64_t size = 0;                                      // transients
    uint64_t alignment = 1;
};

// One resource's use by one pass (merged: at most one per resource per pass).
struct PlanUse
{
    uint32_t resource = 0;
    BarrierSync sync = BarrierSync::None;
    BarrierAccess access = BarrierAccess::NoAccess;
    TextureLayout layout = TextureLayout::Common; // textures
    bool write = false;
};

struct PlanPass
{
    CommandQueueType queue = CommandQueueType::Graphics;
    uint32_t firstUse = 0;
    uint32_t useCount = 0;
};

struct PlanBarrier
{
    uint32_t resource = 0;
    BarrierSync syncBefore = BarrierSync::None;
    BarrierSync syncAfter = BarrierSync::None;
    BarrierAccess accessBefore = BarrierAccess::NoAccess;
    BarrierAccess accessAfter = BarrierAccess::NoAccess;
    TextureLayout layoutBefore = TextureLayout::Undefined; // textures
    TextureLayout layoutAfter = TextureLayout::Undefined;
};

struct PlanPassBarriers
{
    Vector<PlanBarrier> begin; // before the pass's commands
    Vector<PlanBarrier> end;   // after them
};

// One submit. Waits name earlier batches (by index) on other queues.
struct PlanBatch
{
    CommandQueueType queue = CommandQueueType::Graphics;
    Vector<uint32_t> passes;
    Vector<uint32_t> waits;
    bool epilogue = false;           // final layouts only: its barriers are Plan::epilogue
    uint32_t sequence = 0;           // its position among its queue's batches
    int64_t clock[3] = {-1, -1, -1}; // per queue: the last batch sequence finished before this one starts
};

struct Plan
{
    explicit Plan(const HostAllocator* allocator)
        : passes(allocator)
        , passBatch(allocator)
        , batches(allocator)
        , submitOrder(allocator)
        , epilogue(allocator)
        , offsets(allocator)
        , firstPass(allocator)
    {}

    Vector<PlanPassBarriers> passes;
    Vector<uint32_t> passBatch;
    Vector<PlanBatch> batches;
    Vector<uint32_t> submitOrder; // batch indices, in the order to submit them
    Vector<PlanBarrier> epilogue;
    Vector<uint64_t> offsets;   // per resource: transient memory offset
    Vector<uint32_t> firstPass; // per resource: the pass that first uses it, or none
    uint64_t transientSize = 0;
};

// Layouts a queue type can transition from and to (ADR 0049).
bool layoutValidOn(TextureLayout layout, CommandQueueType queue);

// Inputs are already validated: every use's layout suits its pass's queue,
// and every imported texture's initial layout suits the queue that first uses
// it. Returns OutOfHostMemory or Ok.
Status buildPlan(const HostAllocator* allocator, std::span<const PlanResource> resources,
                 std::span<const PlanPass> passes, std::span<const PlanUse> uses, Plan& plan);

} // namespace slag::rg
