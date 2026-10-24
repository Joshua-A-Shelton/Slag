#include <slag/Flags.h>

#include <doctest/doctest.h>

#include <cstdint>

namespace {

enum class TestFlags : uint32_t
{
    None = 0,
    A = 1u << 0,
    B = 1u << 1,
    C = 1u << 31,
};
SLAG_FLAGS(TestFlags)

} // namespace

TEST_CASE("SLAG_FLAGS combines and masks flags")
{
    TestFlags flags = TestFlags::A | TestFlags::C;
    CHECK((flags & TestFlags::A) == TestFlags::A);
    CHECK((flags & TestFlags::B) == TestFlags::None);
    CHECK((flags & TestFlags::C) == TestFlags::C);

    flags &= ~TestFlags::A;
    CHECK(flags == TestFlags::C);
    flags |= TestFlags::B;
    CHECK(static_cast<uint32_t>(flags) == ((1u << 31) | (1u << 1)));
    flags ^= TestFlags::B;
    CHECK(flags == TestFlags::C);
}
