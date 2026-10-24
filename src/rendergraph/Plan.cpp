#include "rendergraph/Plan.h"

#include <utility>

namespace slag::rendergraph {

namespace {

size_t queueIndex(CommandQueueType queue)
{
    return static_cast<size_t>(queue);
}

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return alignment > 1 ? (value + alignment - 1) / alignment * alignment : value;
}

// What a resource's earlier uses left for the next barrier to wait on.
struct ResourceState
{
    TextureLayout layout = TextureLayout::Undefined;
    uint32_t lastPass = none;
    BarrierSync writeSync = BarrierSync::None; // the last write, since which...
    BarrierAccess writeAccess = BarrierAccess::NoAccess;
    BarrierSync readSync = BarrierSync::None; // ...these reads happened
    BarrierAccess readAccess = BarrierAccess::NoAccess;
    BarrierSync visibleSync = BarrierSync::None; // where the write is already visible
    BarrierAccess visibleAccess = BarrierAccess::NoAccess;
    bool deactivated = false; // a transient whose memory a later one took over

    BarrierSync pendingSync() const
    {
        return writeSync | readSync;
    }

    BarrierAccess pendingAccess() const
    {
        return writeAccess | readAccess;
    }

    // After a barrier (or a submit boundary) leading into `use`.
    void restart(const PlanUse& use)
    {
        writeSync = use.write ? use.sync : BarrierSync::None;
        writeAccess = use.write ? use.access : BarrierAccess::NoAccess;
        readSync = use.write ? BarrierSync::None : use.sync;
        readAccess = use.write ? BarrierAccess::NoAccess : use.access;
        visibleSync = BarrierSync::None;
        visibleAccess = BarrierAccess::NoAccess;
    }
};

bool contains(BarrierSync set, BarrierSync bits)
{
    return (set & bits) == bits;
}

bool contains(BarrierAccess set, BarrierAccess bits)
{
    return (set & bits) == bits;
}

PlanBarrier makeBarrier(uint32_t resource, BarrierSync syncBefore, BarrierAccess accessBefore,
                        TextureLayout layoutBefore, BarrierSync syncAfter, BarrierAccess accessAfter,
                        TextureLayout layoutAfter)
{
    PlanBarrier barrier;
    barrier.resource = resource;
    barrier.syncBefore = syncBefore;
    barrier.accessBefore = accessBefore;
    barrier.layoutBefore = layoutBefore;
    barrier.syncAfter = syncAfter;
    barrier.accessAfter = accessAfter;
    barrier.layoutAfter = layoutAfter;
    return barrier;
}

} // namespace

bool layoutValidOn(TextureLayout layout, CommandQueueType queue)
{
    switch (queue)
    {
    case CommandQueueType::Graphics:
        return true;
    case CommandQueueType::Compute:
        return layout == TextureLayout::Common || layout == TextureLayout::ShaderRead ||
               layout == TextureLayout::ShaderWrite || layout == TextureLayout::CopySource ||
               layout == TextureLayout::CopyDest;
    case CommandQueueType::Copy:
        return layout == TextureLayout::Common;
    }
    return false;
}

Status buildPlan(const HostAllocator* allocator, std::span<const PlanResource> resources,
                 std::span<const PlanPass> passes, std::span<const PlanUse> uses, Plan& plan)
{
    const uint32_t passCount = static_cast<uint32_t>(passes.size());
    const uint32_t resourceCount = static_cast<uint32_t>(resources.size());
    Vector<uint32_t> lastUse(allocator);
    Vector<ResourceState> states(allocator);
    if (!plan.passes.reserve(passCount) || !plan.passBatch.reserve(passCount) || !plan.offsets.reserve(resourceCount) ||
        !plan.firstPass.reserve(resourceCount) || !lastUse.reserve(resourceCount) || !states.reserve(resourceCount))
    {
        return Status::OutOfHostMemory;
    }
    for (uint32_t p = 0; p < passCount; ++p)
    {
        PlanPassBarriers barriers;
        barriers.begin = Vector<PlanBarrier>(allocator);
        barriers.end = Vector<PlanBarrier>(allocator);
        (void)plan.passes.pushBack(std::move(barriers)); // reserved
        (void)plan.passBatch.pushBack(none);
    }
    for (uint32_t r = 0; r < resourceCount; ++r)
    {
        (void)plan.offsets.pushBack(0);
        (void)plan.firstPass.pushBack(none);
        (void)lastUse.pushBack(none);
        ResourceState state;
        state.layout = resources[r].transient ? TextureLayout::Undefined : resources[r].initialLayout;
        (void)states.pushBack(state);
    }

    // --- 1. Batches: consecutive passes of a queue share a submit until one
    // needs a resource last used on another queue. That splits both queues'
    // submits, so every wait names an already-closed batch: no cycles, and
    // waits never come before their signals in submission order.
    uint32_t open[3] = {none, none, none};
    uint32_t lastBatch[3] = {none, none, none};
    uint32_t sequenceCount[3] = {0, 0, 0};
    bool failed = false;
    auto close = [&](uint32_t batch) {
        failed = failed || !plan.submitOrder.pushBack(batch);
        open[queueIndex(plan.batches[batch].queue)] = none;
    };
    // What is known finished before new work on `queue` starts.
    auto known = [&](CommandQueueType queue, int64_t clock[3]) {
        size_t q = queueIndex(queue);
        uint32_t from = open[q] != none ? open[q] : lastBatch[q];
        for (size_t k = 0; k < 3; ++k)
        {
            clock[k] = from != none ? plan.batches[from].clock[k] : -1;
        }
        if (open[q] == none && from != none)
        {
            clock[q] = plan.batches[from].sequence; // the previous batch finishes first
        }
    };
    auto create = [&](CommandQueueType queue, const uint32_t deps[3]) -> uint32_t {
        size_t q = queueIndex(queue);
        PlanBatch batch;
        batch.queue = queue;
        batch.passes = Vector<uint32_t>(allocator);
        batch.waits = Vector<uint32_t>(allocator);
        batch.sequence = sequenceCount[q]++;
        known(queue, batch.clock);
        for (size_t k = 0; k < 3; ++k)
        {
            if (deps[k] == none)
            {
                continue;
            }
            const PlanBatch& waited = plan.batches[deps[k]];
            for (size_t j = 0; j < 3; ++j)
            {
                batch.clock[j] = waited.clock[j] > batch.clock[j] ? waited.clock[j] : batch.clock[j];
            }
            batch.clock[k] = static_cast<int64_t>(waited.sequence) > batch.clock[k]
                                 ? static_cast<int64_t>(waited.sequence)
                                 : batch.clock[k];
            failed = failed || !batch.waits.pushBack(deps[k]);
        }
        batch.clock[q] = static_cast<int64_t>(batch.sequence) - 1;
        uint32_t index = static_cast<uint32_t>(plan.batches.size());
        failed = failed || !plan.batches.pushBack(std::move(batch));
        lastBatch[q] = index;
        open[q] = index;
        return index;
    };

    for (uint32_t p = 0; p < passCount && !failed; ++p)
    {
        CommandQueueType queue = passes[p].queue;
        size_t q = queueIndex(queue);
        uint32_t deps[3] = {none, none, none};
        for (uint32_t i = 0; i < passes[p].useCount; ++i)
        {
            uint32_t previous = lastUse[uses[passes[p].firstUse + i].resource];
            if (previous == none)
            {
                continue;
            }
            uint32_t batch = plan.passBatch[previous];
            size_t k = queueIndex(plan.batches[batch].queue);
            if (k != q && (deps[k] == none || plan.batches[batch].sequence > plan.batches[deps[k]].sequence))
            {
                deps[k] = batch;
            }
        }
        int64_t clock[3];
        known(queue, clock);
        bool wait = false;
        for (size_t k = 0; k < 3; ++k)
        {
            if (deps[k] != none && static_cast<int64_t>(plan.batches[deps[k]].sequence) <= clock[k])
            {
                deps[k] = none; // already finished before this queue's work starts
            }
            wait = wait || deps[k] != none;
        }
        for (size_t k = 0; k < 3; ++k)
        {
            if (deps[k] != none && open[k] == deps[k])
            {
                close(deps[k]);
            }
        }
        if (wait && open[q] != none)
        {
            close(open[q]);
        }
        uint32_t batch = open[q] != none ? open[q] : create(queue, deps);
        if (failed)
        {
            break;
        }
        failed = !plan.batches[batch].passes.pushBack(p);
        plan.passBatch[p] = batch;
        for (uint32_t i = 0; i < passes[p].useCount; ++i)
        {
            uint32_t r = uses[passes[p].firstUse + i].resource;
            lastUse[r] = p;
            plan.firstPass[r] = plan.firstPass[r] == none ? p : plan.firstPass[r];
        }
    }
    if (failed)
    {
        return Status::OutOfHostMemory;
    }

    // Whether pass x's work is finished (or ordered by a barrier) before y's.
    auto ordered = [&](uint32_t x, uint32_t y) {
        uint32_t bx = plan.passBatch[x];
        uint32_t by = plan.passBatch[y];
        if (bx == by)
        {
            return x < y;
        }
        return static_cast<int64_t>(plan.batches[bx].sequence) <=
               plan.batches[by].clock[queueIndex(plan.batches[bx].queue)];
    };

    // --- 2. Transient placement: largest first, each at the lowest offset
    // that overlaps no transient whose uses aren't ordered against its own.
    Vector<uint32_t> order(allocator);
    Vector<uint32_t> placed(allocator);
    if (!order.reserve(resourceCount) || !placed.reserve(resourceCount))
    {
        return Status::OutOfHostMemory;
    }
    for (uint32_t r = 0; r < resourceCount; ++r)
    {
        if (resources[r].transient && plan.firstPass[r] != none)
        {
            size_t i = order.size();
            (void)order.pushBack(r);
            while (i > 0 && resources[order[i - 1]].size < resources[r].size)
            {
                order[i] = order[i - 1];
                --i;
            }
            order[i] = r;
        }
    }
    auto conflicts = [&](uint32_t a, uint32_t b) {
        return !ordered(lastUse[a], plan.firstPass[b]) && !ordered(lastUse[b], plan.firstPass[a]);
    };
    auto overlaps = [&](uint32_t a, uint64_t offset, uint64_t size) {
        return offset < plan.offsets[a] + resources[a].size && plan.offsets[a] < offset + size;
    };
    for (uint32_t t : order)
    {
        uint64_t best = UINT64_MAX;
        for (size_t candidate = 0; candidate <= placed.size(); ++candidate)
        {
            uint64_t start = 0;
            if (candidate < placed.size())
            {
                uint32_t c = placed[candidate];
                if (!conflicts(c, t))
                {
                    continue;
                }
                start = plan.offsets[c] + resources[c].size;
            }
            uint64_t offset = alignUp(start, resources[t].alignment);
            bool free = true;
            for (uint32_t c : placed)
            {
                free = free && !(conflicts(c, t) && overlaps(c, offset, resources[t].size));
            }
            best = free && offset < best ? offset : best;
        }
        plan.offsets[t] = best;
        plan.transientSize =
            best + resources[t].size > plan.transientSize ? best + resources[t].size : plan.transientSize;
        (void)placed.pushBack(t);
    }

    // --- 3. Barriers, walking the passes in order.
    for (uint32_t p = 0; p < passCount && !failed; ++p)
    {
        CommandQueueType queue = passes[p].queue;
        uint32_t batch = plan.passBatch[p];
        PlanPassBarriers& out = plan.passes[p];
        for (uint32_t i = 0; i < passes[p].useCount && !failed; ++i)
        {
            const PlanUse& use = uses[passes[p].firstUse + i];
            const uint32_t r = use.resource;
            const PlanResource& resource = resources[r];
            ResourceState& state = states[r];
            auto add = [&](Vector<PlanBarrier>& list, const PlanBarrier& barrier) {
                failed = failed || !list.pushBack(barrier);
            };
            if (state.lastPass == none && resource.transient)
            {
                // Memory last used by transients that ended earlier in this
                // submit: wait for their accesses, flush and deactivate them,
                // then activate this one from Undefined (Enhanced Barriers
                // aliasing). Earlier submits need nothing.
                BarrierSync aliasSync = BarrierSync::None;
                for (uint32_t c : placed)
                {
                    if (c == r || !overlaps(c, plan.offsets[r], resource.size) || !ordered(lastUse[c], p) ||
                        plan.passBatch[lastUse[c]] != batch)
                    {
                        continue;
                    }
                    ResourceState& old = states[c];
                    aliasSync = aliasSync | old.pendingSync();
                    if (!old.deactivated)
                    {
                        // SyncAfter None: D3D12 allows no other with NO_ACCESS
                        // unless the layout becomes Undefined, which Vulkan
                        // forbids. The new resource's barrier waits on the
                        // old stages itself.
                        add(out.begin, makeBarrier(c, old.pendingSync(), old.pendingAccess(), old.layout,
                                                   BarrierSync::None, BarrierAccess::NoAccess, old.layout));
                        old.deactivated = true;
                    }
                }
                if (resource.texture)
                {
                    add(out.begin, makeBarrier(r, aliasSync, BarrierAccess::NoAccess, TextureLayout::Undefined,
                                               use.sync, use.access, use.layout));
                }
                else if (aliasSync != BarrierSync::None)
                {
                    add(out.begin, makeBarrier(r, aliasSync, BarrierAccess::NoAccess, TextureLayout::Undefined,
                                               use.sync, use.access, TextureLayout::Undefined));
                }
                state.restart(use);
            }
            else if (state.lastPass == none)
            {
                // Imported: earlier work is in earlier submits (ADR 0049).
                if (resource.texture && state.layout != use.layout)
                {
                    add(out.begin, makeBarrier(r, BarrierSync::None, BarrierAccess::NoAccess, state.layout, use.sync,
                                               use.access, use.layout));
                }
                state.restart(use);
            }
            else if (plan.passBatch[state.lastPass] == batch)
            {
                bool layoutChange = resource.texture && state.layout != use.layout;
                if (use.write || layoutChange)
                {
                    add(out.begin, makeBarrier(r, state.pendingSync(), state.pendingAccess(), state.layout, use.sync,
                                               use.access, use.layout));
                    state.restart(use);
                    if (!use.write)
                    {
                        state.visibleSync = use.sync;
                        state.visibleAccess = use.access;
                    }
                }
                else
                {
                    // Read after read in the same layout: only a write not
                    // yet visible to this read needs a barrier.
                    if (state.writeAccess != BarrierAccess::NoAccess &&
                        !(contains(state.visibleSync, use.sync) && contains(state.visibleAccess, use.access)))
                    {
                        add(out.begin, makeBarrier(r, state.writeSync, state.writeAccess, state.layout, use.sync,
                                                   use.access, state.layout));
                        state.visibleSync = state.visibleSync | use.sync;
                        state.visibleAccess = state.visibleAccess | use.access;
                    }
                    state.readSync = state.readSync | use.sync;
                    state.readAccess = state.readAccess | use.access;
                }
            }
            else
            {
                // An earlier submit: finished and visible (ADR 0049), at most
                // a layout change. A layout this queue can't transition from
                // is changed at the end of the last use, on its queue.
                uint32_t lastBatchIndex = plan.passBatch[state.lastPass];
                CommandQueueType lastQueue = plan.batches[lastBatchIndex].queue;
                if (resource.texture && state.layout != use.layout && !layoutValidOn(state.layout, queue))
                {
                    TextureLayout handoff = layoutValidOn(use.layout, lastQueue) ? use.layout : TextureLayout::Common;
                    add(plan.passes[state.lastPass].end,
                        makeBarrier(r, state.pendingSync(), state.pendingAccess(), state.layout, BarrierSync::None,
                                    BarrierAccess::NoAccess, handoff));
                    state.layout = handoff;
                }
                if (resource.texture && state.layout != use.layout)
                {
                    add(out.begin, makeBarrier(r, BarrierSync::None, BarrierAccess::NoAccess, state.layout, use.sync,
                                               use.access, use.layout));
                }
                state.restart(use);
            }
            state.lastPass = p;
            if (resource.texture)
            {
                state.layout = use.layout;
            }
        }
    }
    if (failed)
    {
        return Status::OutOfHostMemory;
    }

    // --- 4. Final layouts of imported textures: at the end of their last use
    // when that queue can make the change, otherwise in an epilogue submit on
    // the graphics queue.
    uint32_t epilogueDeps[3] = {none, none, none};
    for (uint32_t r = 0; r < resourceCount && !failed; ++r)
    {
        const PlanResource& resource = resources[r];
        ResourceState& state = states[r];
        if (!resource.texture || resource.transient || state.layout == resource.finalLayout)
        {
            continue;
        }
        if (state.lastPass != none &&
            layoutValidOn(resource.finalLayout, plan.batches[plan.passBatch[state.lastPass]].queue))
        {
            failed = !plan.passes[state.lastPass].end.pushBack(
                makeBarrier(r, state.pendingSync(), state.pendingAccess(), state.layout, BarrierSync::None,
                            BarrierAccess::NoAccess, resource.finalLayout));
        }
        else
        {
            failed =
                !plan.epilogue.pushBack(makeBarrier(r, BarrierSync::None, BarrierAccess::NoAccess, state.layout,
                                                    BarrierSync::None, BarrierAccess::NoAccess, resource.finalLayout));
            if (state.lastPass != none)
            {
                uint32_t b = plan.passBatch[state.lastPass];
                size_t k = queueIndex(plan.batches[b].queue);
                if (epilogueDeps[k] == none || plan.batches[b].sequence > plan.batches[epilogueDeps[k]].sequence)
                {
                    epilogueDeps[k] = b;
                }
            }
        }
        state.layout = resource.finalLayout;
    }
    for (size_t k = 0; k < 3; ++k)
    {
        if (open[k] != none)
        {
            close(open[k]);
        }
    }
    if (!plan.epilogue.empty() && !failed)
    {
        int64_t clock[3];
        known(CommandQueueType::Graphics, clock);
        for (size_t k = 0; k < 3; ++k)
        {
            if (epilogueDeps[k] != none && static_cast<int64_t>(plan.batches[epilogueDeps[k]].sequence) <= clock[k])
            {
                epilogueDeps[k] = none;
            }
        }
        uint32_t batch = create(CommandQueueType::Graphics, epilogueDeps);
        if (!failed)
        {
            plan.batches[batch].epilogue = true;
            close(batch);
        }
    }
    return failed ? Status::OutOfHostMemory : Status::Ok;
}

} // namespace slag::rendergraph
