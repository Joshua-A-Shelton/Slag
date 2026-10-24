#pragma once

// Backend-independent facts about each PixelFormat (ADR 0022).

#include <slag/Barriers.h>
#include <slag/PixelFormat.h>

#include <cstddef>
#include <cstdint>

namespace slag {

constexpr size_t pixelFormatCount = static_cast<size_t>(PixelFormat::BC7UnormSrgb) + 1;

enum class FormatTraits : uint8_t
{
    None = 0,
    Depth = 1u << 0,
    Stencil = 1u << 1,
    Compressed = 1u << 2, // 4x4 blocks
    Srgb = 1u << 3,
    Integer = 1u << 4,
    VertexOnly = 1u << 5,
};
SLAG_FLAGS(FormatTraits)

struct FormatInfo
{
    const char* name;
    uint8_t blockBytes; // bytes per texel, or per 4x4 block when compressed
    FormatTraits traits;
    // Never reported, even if a backend offers it (the "—" entries).
    FormatSupport allowed;
    // Every baseline device supports these (● and ◐, docs/format-support.md).
    FormatSupport guaranteed;
};

const FormatInfo& formatInfo(PixelFormat format);

inline bool hasTraits(PixelFormat format, FormatTraits traits)
{
    return (formatInfo(format).traits & traits) == traits;
}

// 4 for block-compressed formats, else 1.
inline uint32_t blockDimension(PixelFormat format)
{
    return hasTraits(format, FormatTraits::Compressed) ? 4 : 1;
}

// Bytes per texel (or block) of one plane in a buffer copy: depth/stencil
// formats copy depth as 2 or 4 bytes and stencil as 1 (ADR 0048). Color
// means the only plane of a single-plane format.
uint32_t copyBlockBytes(PixelFormat format, TexturePlane plane);

// Same texel size and type family, for castable formats (ADR 0022): sRGB and
// linear variants, or uncompressed formats with the same bits per texel.
bool formatsCastable(PixelFormat a, PixelFormat b);

// The first format whose guaranteed support a device lacks, or Undefined.
// `support` has pixelFormatCount entries.
PixelFormat missingGuaranteedSupport(const FormatSupport* support, FormatSupport& missing);

struct InstanceCommon;

// Logs and returns false when a device lacks guaranteed format support
// (the ◐ baseline checks, ADR 0012).
bool meetsFormatBaseline(const InstanceCommon& instance, const char* deviceName, const FormatSupport* support);

// "Sampled, RenderTarget" for logging; truncated to fit.
void describeFormatSupport(FormatSupport support, char* out, size_t outSize);

} // namespace slag
