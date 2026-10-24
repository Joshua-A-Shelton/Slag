#include "common/Formats.h"

#include <doctest/doctest.h>

TEST_CASE("Every guaranteed format capability is also allowed")
{
    for (size_t i = 0; i < slag::pixelFormatCount; ++i)
    {
        const slag::FormatInfo& info = slag::formatInfo(static_cast<slag::PixelFormat>(i));
        CAPTURE(info.name);
        CHECK((info.guaranteed & ~info.allowed) == slag::FormatSupport::None);
    }
}

TEST_CASE("Castable formats: sRGB pairs and equal texel sizes, no depth or block-compressed casts")
{
    using slag::PixelFormat;
    CHECK(slag::formatsCastable(PixelFormat::R8G8B8A8Unorm, PixelFormat::R8G8B8A8UnormSrgb));
    CHECK(slag::formatsCastable(PixelFormat::BC7Unorm, PixelFormat::BC7UnormSrgb));
    CHECK(slag::formatsCastable(PixelFormat::R32Float, PixelFormat::R8G8B8A8Uint));
    CHECK(slag::formatsCastable(PixelFormat::R16G16B16A16Float, PixelFormat::R32G32Uint));
    CHECK_FALSE(slag::formatsCastable(PixelFormat::R32Float, PixelFormat::R16Float));
    CHECK_FALSE(slag::formatsCastable(PixelFormat::D32Float, PixelFormat::R32Float));
    CHECK_FALSE(slag::formatsCastable(PixelFormat::BC1Unorm, PixelFormat::R32G32Uint));
    CHECK_FALSE(slag::formatsCastable(PixelFormat::R32G32B32Float, PixelFormat::R32G32B32Uint));
}

TEST_CASE("missingGuaranteedSupport finds the first gap")
{
    slag::FormatSupport support[slag::pixelFormatCount];
    for (size_t i = 0; i < slag::pixelFormatCount; ++i)
    {
        support[i] = slag::formatInfo(static_cast<slag::PixelFormat>(i)).guaranteed;
    }
    slag::FormatSupport missing = slag::FormatSupport::None;
    CHECK(slag::missingGuaranteedSupport(support, missing) == slag::PixelFormat::Undefined);

    support[static_cast<size_t>(slag::PixelFormat::R16Unorm)] &= ~slag::FormatSupport::Blend;
    CHECK(slag::missingGuaranteedSupport(support, missing) == slag::PixelFormat::R16Unorm);
    CHECK(missing == slag::FormatSupport::Blend);
}
