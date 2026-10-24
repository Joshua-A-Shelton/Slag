// The render graph planner (ADR 0072), without a GPU: submits and waits,
// barriers, handoffs between queues, final layouts, and transient aliasing.

#include "rendergraph/Plan.h"

#include <doctest/doctest.h>

#include <vector>

using namespace slag;
using namespace slag::rg;

namespace {

struct Graph
{
    std::vector<PlanResource> resources;
    std::vector<PlanPass> passes;
    std::vector<PlanUse> uses;
    HostAllocator allocator;
    Plan plan{&allocator};

    uint32_t transient(bool texture = true, uint64_t size = 1024)
    {
        resources.push_back({.texture = texture, .transient = true, .size = size, .alignment = 256});
        return static_cast<uint32_t>(resources.size() - 1);
    }

    uint32_t imported(TextureLayout initial, TextureLayout final, bool texture = true)
    {
        resources.push_back({.texture = texture, .initialLayout = initial, .finalLayout = final});
        return static_cast<uint32_t>(resources.size() - 1);
    }

    void pass(CommandQueueType queue, std::vector<PlanUse> passUses)
    {
        passes.push_back({queue, static_cast<uint32_t>(uses.size()), static_cast<uint32_t>(passUses.size())});
        uses.insert(uses.end(), passUses.begin(), passUses.end());
    }

    void build()
    {
        REQUIRE(buildPlan(&allocator, resources, passes, uses, plan) == Status::Ok);
    }

    const PlanBatch& batchOf(uint32_t pass) const
    {
        return plan.batches[plan.passBatch[pass]];
    }
};

PlanUse renderTarget(uint32_t r)
{
    return {r, BarrierSync::RenderTarget, BarrierAccess::RenderTarget, TextureLayout::RenderTarget, true};
}

PlanUse shaderRead(uint32_t r, BarrierSync sync = BarrierSync::AllShading)
{
    return {r, sync, BarrierAccess::ShaderRead, TextureLayout::ShaderRead, false};
}

PlanUse shaderWrite(uint32_t r, BarrierSync sync = BarrierSync::ComputeShading)
{
    return {r, sync, BarrierAccess::ShaderWrite, TextureLayout::ShaderWrite, true};
}

PlanUse copySource(uint32_t r, TextureLayout layout = TextureLayout::CopySource)
{
    return {r, BarrierSync::Copy, BarrierAccess::CopySource, layout, false};
}

} // namespace

TEST_CASE("render graph plan: one queue, write then read")
{
    Graph g;
    uint32_t target = g.transient();
    g.pass(CommandQueueType::Graphics, {renderTarget(target)});
    g.pass(CommandQueueType::Graphics, {shaderRead(target)});
    g.build();
    REQUIRE(g.plan.batches.size() == 1);
    CHECK(g.plan.batches[0].passes.size() == 2);
    CHECK(g.plan.batches[0].waits.empty());

    REQUIRE(g.plan.passes[0].begin.size() == 1); // activated from Undefined
    CHECK(g.plan.passes[0].begin[0].layoutBefore == TextureLayout::Undefined);
    CHECK(g.plan.passes[0].begin[0].layoutAfter == TextureLayout::RenderTarget);
    CHECK(g.plan.passes[0].begin[0].syncBefore == BarrierSync::None);

    REQUIRE(g.plan.passes[1].begin.size() == 1);
    const PlanBarrier& barrier = g.plan.passes[1].begin[0];
    CHECK(barrier.syncBefore == BarrierSync::RenderTarget);
    CHECK(barrier.accessBefore == BarrierAccess::RenderTarget);
    CHECK(barrier.layoutBefore == TextureLayout::RenderTarget);
    CHECK(barrier.syncAfter == BarrierSync::AllShading);
    CHECK(barrier.accessAfter == BarrierAccess::ShaderRead);
    CHECK(barrier.layoutAfter == TextureLayout::ShaderRead);
}

TEST_CASE("render graph plan: reads share a write's barrier until a new stage needs it")
{
    Graph g;
    uint32_t buffer = g.transient(false);
    g.pass(CommandQueueType::Compute, {shaderWrite(buffer)});
    g.pass(CommandQueueType::Compute, {shaderRead(buffer, BarrierSync::ComputeShading)});
    g.pass(CommandQueueType::Compute, {shaderRead(buffer, BarrierSync::ComputeShading)});
    g.pass(CommandQueueType::Compute, {copySource(buffer)});
    g.pass(CommandQueueType::Compute, {shaderWrite(buffer)});
    g.build();
    CHECK(g.plan.passes[1].begin.size() == 1);
    CHECK(g.plan.passes[2].begin.empty());       // already visible to compute reads
    REQUIRE(g.plan.passes[3].begin.size() == 1); // copies need their own visibility
    CHECK(g.plan.passes[3].begin[0].syncBefore == BarrierSync::ComputeShading);
    CHECK(g.plan.passes[3].begin[0].accessBefore == BarrierAccess::ShaderWrite);
    // Write after the reads waits for all of them.
    REQUIRE(g.plan.passes[4].begin.size() == 1);
    CHECK(g.plan.passes[4].begin[0].syncBefore == (BarrierSync::ComputeShading | BarrierSync::Copy));
}

TEST_CASE("render graph plan: a use on another queue splits submits and waits")
{
    Graph g;
    uint32_t a = g.transient(false);
    uint32_t b = g.transient(false);
    g.pass(CommandQueueType::Graphics, {shaderWrite(a, BarrierSync::AllShading)});
    g.pass(CommandQueueType::Compute, {shaderRead(a, BarrierSync::ComputeShading), shaderWrite(b)});
    g.pass(CommandQueueType::Graphics, {shaderRead(b)});
    g.build();
    REQUIRE(g.plan.batches.size() == 3);
    CHECK(g.plan.passBatch[0] != g.plan.passBatch[2]);
    const PlanBatch& compute = g.batchOf(1);
    REQUIRE(compute.waits.size() == 1);
    CHECK(compute.waits[0] == g.plan.passBatch[0]);
    const PlanBatch& last = g.batchOf(2);
    REQUIRE(last.waits.size() == 1);
    CHECK(last.waits[0] == g.plan.passBatch[1]);
    // Every wait names a batch submitted earlier.
    REQUIRE(g.plan.submitOrder.size() == 3);
    CHECK(g.plan.submitOrder[0] == g.plan.passBatch[0]);
    CHECK(g.plan.submitOrder[1] == g.plan.passBatch[1]);
    CHECK(g.plan.submitOrder[2] == g.plan.passBatch[2]);
    // Buffers need no barriers across submits (ADR 0049).
    CHECK(g.plan.passes[1].begin.empty());
    CHECK(g.plan.passes[2].begin.empty());
}

TEST_CASE("render graph plan: a layout the next queue can't use is changed before the handoff")
{
    Graph g;
    uint32_t target = g.transient();
    g.pass(CommandQueueType::Graphics, {renderTarget(target)});
    g.pass(CommandQueueType::Compute, {shaderRead(target, BarrierSync::ComputeShading)});
    g.build();
    REQUIRE(g.plan.passes[0].end.size() == 1);
    const PlanBarrier& handoff = g.plan.passes[0].end[0];
    CHECK(handoff.layoutBefore == TextureLayout::RenderTarget);
    CHECK(handoff.layoutAfter == TextureLayout::ShaderRead); // compute can read it there
    CHECK(handoff.syncBefore == BarrierSync::RenderTarget);
    CHECK(handoff.syncAfter == BarrierSync::None);
    CHECK(g.plan.passes[1].begin.empty());

    Graph copy;
    uint32_t texture = copy.transient();
    copy.pass(CommandQueueType::Graphics, {renderTarget(texture)});
    copy.pass(CommandQueueType::Copy, {copySource(texture, TextureLayout::Common)});
    copy.build();
    REQUIRE(copy.plan.passes[0].end.size() == 1);
    CHECK(copy.plan.passes[0].end[0].layoutAfter == TextureLayout::Common);
    CHECK(copy.plan.passes[1].begin.empty());
}

TEST_CASE("render graph plan: a later submit on the same queue only changes layouts")
{
    Graph g;
    uint32_t buffer = g.transient(false);
    uint32_t texture = g.transient();
    g.pass(CommandQueueType::Graphics, {renderTarget(texture), shaderWrite(buffer, BarrierSync::AllShading)});
    g.pass(CommandQueueType::Compute, {shaderRead(buffer, BarrierSync::ComputeShading)});
    g.pass(CommandQueueType::Graphics, {shaderRead(texture), shaderRead(buffer)});
    g.build();
    REQUIRE(g.plan.passes[2].begin.size() == 1);
    const PlanBarrier& barrier = g.plan.passes[2].begin[0];
    CHECK(barrier.resource == texture);
    CHECK(barrier.syncBefore == BarrierSync::None);
    CHECK(barrier.layoutBefore == TextureLayout::RenderTarget);
    CHECK(barrier.layoutAfter == TextureLayout::ShaderRead);
}

TEST_CASE("render graph plan: final layouts of imported textures")
{
    Graph g;
    uint32_t backbuffer = g.imported(TextureLayout::Undefined, TextureLayout::Present);
    uint32_t written = g.imported(TextureLayout::Common, TextureLayout::Present);
    g.pass(CommandQueueType::Graphics, {renderTarget(backbuffer)});
    g.pass(CommandQueueType::Compute, {shaderWrite(written)});
    g.build();
    REQUIRE(g.plan.passes[0].end.size() == 1); // graphics can make Present itself
    CHECK(g.plan.passes[0].end[0].layoutAfter == TextureLayout::Present);
    CHECK(g.plan.passes[0].end[0].syncAfter == BarrierSync::None);
    // Compute can't: an epilogue on the graphics queue, after the compute work.
    REQUIRE(g.plan.epilogue.size() == 1);
    CHECK(g.plan.epilogue[0].resource == written);
    CHECK(g.plan.epilogue[0].layoutBefore == TextureLayout::ShaderWrite);
    uint32_t epilogue = g.plan.submitOrder[g.plan.submitOrder.size() - 1];
    CHECK(g.plan.batches[epilogue].epilogue);
    CHECK(g.plan.batches[epilogue].queue == CommandQueueType::Graphics);
    REQUIRE(g.plan.batches[epilogue].waits.size() == 1);
    CHECK(g.plan.batches[epilogue].waits[0] == g.plan.passBatch[1]);
}

TEST_CASE("render graph plan: transients share memory when their uses are ordered")
{
    Graph g;
    uint32_t a = g.transient(true, 4096);
    uint32_t b = g.transient(true, 4096);
    uint32_t c = g.transient(true, 1024);
    g.pass(CommandQueueType::Graphics, {renderTarget(a)});
    g.pass(CommandQueueType::Graphics, {shaderRead(a), renderTarget(c)});
    g.pass(CommandQueueType::Graphics, {renderTarget(b), shaderRead(c)});
    g.build();
    CHECK(g.plan.offsets[a] == g.plan.offsets[b]);
    CHECK(g.plan.offsets[c] >= 4096); // overlaps both lifetimes
    CHECK(g.plan.transientSize == 4096 + 1024);
    // b takes a's memory in the same submit: a is waited for and deactivated,
    // b activated from Undefined after it.
    bool deactivated = false;
    bool activated = false;
    for (const PlanBarrier& barrier : g.plan.passes[2].begin)
    {
        if (barrier.resource == a)
        {
            deactivated =
                barrier.accessAfter == BarrierAccess::NoAccess && barrier.syncBefore == BarrierSync::AllShading;
        }
        if (barrier.resource == b)
        {
            activated = barrier.layoutBefore == TextureLayout::Undefined &&
                        barrier.syncBefore == BarrierSync::AllShading &&
                        barrier.accessBefore == BarrierAccess::NoAccess;
        }
    }
    CHECK(deactivated);
    CHECK(activated);
}

TEST_CASE("render graph plan: transients on unordered queues don't share memory")
{
    Graph g;
    uint32_t a = g.transient(false, 4096);
    uint32_t b = g.transient(false, 4096);
    g.pass(CommandQueueType::Graphics, {shaderWrite(a, BarrierSync::AllShading)});
    g.pass(CommandQueueType::Compute, {shaderWrite(b)}); // may run alongside the graphics pass
    g.build();
    CHECK(g.plan.offsets[a] != g.plan.offsets[b]);
    CHECK(g.plan.transientSize == 8192);

    Graph ordered;
    uint32_t x = ordered.transient(false, 4096);
    uint32_t y = ordered.transient(false, 4096);
    uint32_t link = ordered.transient(false, 256);
    ordered.pass(CommandQueueType::Graphics,
                 {shaderWrite(x, BarrierSync::AllShading), shaderWrite(link, BarrierSync::AllShading)});
    ordered.pass(CommandQueueType::Compute, {shaderRead(link, BarrierSync::ComputeShading), shaderWrite(y)});
    ordered.build();
    CHECK(ordered.plan.offsets[x] == ordered.plan.offsets[y]); // the wait orders them
    CHECK(ordered.plan.passes[1].begin.empty());               // and makes barriers unnecessary
}
