#include "common/Vector.h"

#include <doctest/doctest.h>

#include <cstdlib>

namespace {

struct CountingAllocator
{
    int allocations = 0;
    int frees = 0;
    int failAfter = -1; // -1: never fail
};

void* countingAllocate(size_t size, size_t, void* userData)
{
    auto* counter = static_cast<CountingAllocator*>(userData);
    if (counter->failAfter >= 0 && counter->allocations >= counter->failAfter)
    {
        return nullptr;
    }
    ++counter->allocations;
    return std::malloc(size);
}

void countingFree(void* pointer, size_t, void* userData)
{
    ++static_cast<CountingAllocator*>(userData)->frees;
    std::free(pointer);
}

} // namespace

TEST_CASE("Vector grows through the allocator and frees everything")
{
    CountingAllocator counter;
    {
        slag::HostAllocator allocator({countingAllocate, countingFree, &counter});
        slag::Vector<int> values(&allocator);
        for (int i = 0; i < 100; ++i)
        {
            REQUIRE(values.pushBack(i));
        }
        CHECK(values.size() == 100);
        CHECK(values[57] == 57);
        CHECK(counter.allocations > 0);
    }
    CHECK(counter.allocations == counter.frees);
}

TEST_CASE("Vector keeps its capacity across clear")
{
    slag::Vector<int> values;
    REQUIRE(values.reserve(16));
    REQUIRE(values.pushBack(1));
    values.clear();
    CHECK(values.empty());
    CHECK(values.capacity() == 16);
}

TEST_CASE("Vector reports allocation failure instead of aborting")
{
    CountingAllocator counter;
    counter.failAfter = 1;
    slag::HostAllocator allocator({countingAllocate, countingFree, &counter});
    slag::Vector<int> values(&allocator);
    for (int i = 0; i < 4; ++i)
    {
        REQUIRE(values.pushBack(i)); // first allocation holds 4
    }
    CHECK_FALSE(values.pushBack(4)); // growing needs a second allocation
    CHECK(values.size() == 4);
}
