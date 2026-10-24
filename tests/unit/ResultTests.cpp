#include <slag/Result.h>

#include <doctest/doctest.h>

using slag::Result;
using slag::Status;

TEST_CASE("Result holds a value on success")
{
    Result<int> result = 42;
    CHECK(static_cast<bool>(result));
    CHECK(result.status() == Status::Ok);
    CHECK(result.value() == 42);
}

TEST_CASE("Result treats non-error statuses as success")
{
    Result<int> suboptimal(Status::Suboptimal, 7);
    CHECK(static_cast<bool>(suboptimal));
    CHECK(suboptimal.value() == 7);
    CHECK(static_cast<bool>(Result<void>(Status::Timeout)));
}

TEST_CASE("Result reports errors")
{
    Result<int> result = Status::OutOfHostMemory;
    CHECK_FALSE(static_cast<bool>(result));
    CHECK(result.status() == Status::OutOfHostMemory);
    CHECK_FALSE(static_cast<bool>(Result<void>(Status::DeviceLost)));
    CHECK(static_cast<bool>(Result<void>()));
}
