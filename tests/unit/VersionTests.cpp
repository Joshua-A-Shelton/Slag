#include <slag/Version.h>

#include <doctest/doctest.h>

TEST_CASE("SLAG_VERSION packs major, minor and patch like VK_MAKE_API_VERSION")
{
    CHECK((SLAG_VERSION >> 22) == SLAG_VERSION_MAJOR);
    CHECK(((SLAG_VERSION >> 12) & 0x3FF) == SLAG_VERSION_MINOR);
    CHECK((SLAG_VERSION & 0xFFF) == SLAG_VERSION_PATCH);
}
