#include "common/Tlsf.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

namespace {

struct Live
{
    uint32_t node;
    uint64_t offset;
    uint64_t size;
};

// Allocations never overlap, stay in range, and respect alignment; free
// nodes never touch each other (they coalesce).
void checkInvariants(const slag::Tlsf& tlsf, const std::vector<Live>& live, uint64_t capacity)
{
    std::vector<Live> sorted = live;
    std::sort(sorted.begin(), sorted.end(), [](const Live& a, const Live& b) { return a.offset < b.offset; });
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        REQUIRE(sorted[i].offset + sorted[i].size <= capacity);
        if (i > 0)
        {
            REQUIRE(sorted[i - 1].offset + sorted[i - 1].size <= sorted[i].offset);
        }
    }
    for (const slag::Tlsf::Node& node : tlsf.nodes())
    {
        if (node.unused || !node.isFree)
        {
            continue;
        }
        if (node.nextPhysical != slag::Tlsf::invalid)
        {
            REQUIRE_FALSE(tlsf.nodes()[node.nextPhysical].isFree);
        }
    }
}

} // namespace

TEST_CASE("Tlsf allocates aligned, non-overlapping ranges and coalesces back to one free range")
{
    slag::HostAllocator allocator;
    slag::Tlsf tlsf(&allocator);
    constexpr uint64_t capacity = 256ull << 20;
    REQUIRE(tlsf.init(capacity));
    CHECK(tlsf.empty());
    CHECK(tlsf.freeBytes() == capacity);

    std::mt19937_64 random(1234);
    const uint64_t alignments[] = {4096, 65536, 4u << 20};
    std::vector<Live> live;
    int owner = 0;
    for (int step = 0; step < 4000; ++step)
    {
        bool allocate = live.empty() || random() % 3 != 0;
        if (allocate)
        {
            uint64_t size = (random() % (8u << 20)) + 1;
            uint64_t alignment = alignments[random() % 3];
            uint32_t node = 0;
            uint64_t offset = 0;
            slag::Status status = tlsf.allocate(size, alignment, &owner, step % 2 == 0, node, offset);
            if (status == slag::Status::Unsupported)
            {
                continue; // full
            }
            REQUIRE(status == slag::Status::Ok);
            CHECK(offset % alignment == 0);
            CHECK(tlsf.nodes()[node].owner == &owner);
            CHECK(tlsf.nodes()[node].alignment == alignment);
            live.push_back({node, offset, tlsf.nodes()[node].size});
        }
        else
        {
            size_t index = random() % live.size();
            tlsf.free(live[index].node);
            live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
        }
        if (step % 100 == 0)
        {
            checkInvariants(tlsf, live, capacity);
        }
    }
    checkInvariants(tlsf, live, capacity);
    for (const Live& allocation : live)
    {
        tlsf.free(allocation.node);
    }
    CHECK(tlsf.empty());
    CHECK(tlsf.freeBytes() == capacity);

    // Everything coalesced: the whole range fits again.
    uint32_t node = 0;
    uint64_t offset = 0;
    CHECK(tlsf.allocate(capacity, 4u << 20, nullptr, false, node, offset) == slag::Status::Ok);
    CHECK(offset == 0);
}

TEST_CASE("Tlsf reports a full block")
{
    slag::HostAllocator allocator;
    slag::Tlsf tlsf(&allocator);
    REQUIRE(tlsf.init(1 << 20));
    uint32_t node = 0;
    uint64_t offset = 0;
    CHECK(tlsf.allocate(1 << 20, 65536, nullptr, false, node, offset) == slag::Status::Ok);
    CHECK(tlsf.allocate(256, 256, nullptr, false, node, offset) == slag::Status::Unsupported);
}
