#include "common/Formats.h"
#include "common/Logging.h"
#include "common/Objects.h"

#include <cstdio>

namespace slag {

namespace {

using enum FormatSupport;
using T = FormatTraits;

constexpr FormatSupport msaa = Multisample2 | Multisample4 | Multisample8;
constexpr FormatSupport anyBuffer = VertexBuffer | TypedBuffer | RWTypedBuffer;
// Color formats: everything but depth/stencil. Atomics only on R32Uint/R32Sint.
constexpr FormatSupport floatColor =
    Sampled | LinearFilter | RenderTarget | Blend | msaa | RWTexture | RWTextureLoad | anyBuffer;
constexpr FormatSupport intColor = Sampled | RenderTarget | msaa | RWTexture | RWTextureLoad | anyBuffer;
constexpr FormatSupport srgbColor = Sampled | LinearFilter | RenderTarget | Blend | msaa;
constexpr FormatSupport depth = Sampled | LinearFilter | msaa | DepthStencil;
constexpr FormatSupport sampledOnly = Sampled | LinearFilter;

// Guaranteed sets, by row of docs/format-support.md.
constexpr FormatSupport S = Sampled;
constexpr FormatSupport F = LinearFilter;
constexpr FormatSupport R = RenderTarget;
constexpr FormatSupport B = Blend;
constexpr FormatSupport M = Multisample4;
constexpr FormatSupport W = RWTexture;
constexpr FormatSupport L = RWTextureLoad;
constexpr FormatSupport A = Atomics;
constexpr FormatSupport D = DepthStencil;
constexpr FormatSupport V = VertexBuffer;
constexpr FormatSupport TB = TypedBuffer;
constexpr FormatSupport U = RWTypedBuffer;

// Indexed by PixelFormat.
constexpr FormatInfo formats[] = {
    {"Undefined", 0, T::None, None, None},

    {"R8Unorm", 1, T::None, floatColor, S | F | R | B | M | W | TB},
    {"R8Snorm", 1, T::None, floatColor, S | F | W | TB},
    {"R8Uint", 1, T::Integer, intColor, S | R | M | W | TB},
    {"R8Sint", 1, T::Integer, intColor, S | R | M | W | TB},
    {"R8G8Unorm", 2, T::None, floatColor, S | F | R | B | M | W | V | TB},
    {"R8G8Snorm", 2, T::None, floatColor, S | F | W | V | TB},
    {"R8G8Uint", 2, T::Integer, intColor, S | R | M | W | V | TB},
    {"R8G8Sint", 2, T::Integer, intColor, S | R | M | W | V | TB},
    {"R8G8B8A8Unorm", 4, T::None, floatColor, S | F | R | B | M | W | V | TB | U},
    {"R8G8B8A8UnormSrgb", 4, T::Srgb, srgbColor, S | F | R | B | M},
    {"R8G8B8A8Snorm", 4, T::None, floatColor, S | F | W | V | TB | U},
    {"R8G8B8A8Uint", 4, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R8G8B8A8Sint", 4, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"B8G8R8A8Unorm", 4, T::None, floatColor, S | F | R | B | M | V | TB},
    {"B8G8R8A8UnormSrgb", 4, T::Srgb, srgbColor, S | F | R | B | M},

    {"R16Unorm", 2, T::None, floatColor, S | F | R | B | M | W},
    {"R16Snorm", 2, T::None, floatColor, S | F | W},
    {"R16Uint", 2, T::Integer, intColor, S | R | M | W | TB},
    {"R16Sint", 2, T::Integer, intColor, S | R | M | W | TB},
    {"R16Float", 2, T::None, floatColor, S | F | R | B | M | W | TB},
    {"R16G16Unorm", 4, T::None, floatColor, S | F | R | B | M | W | V},
    {"R16G16Snorm", 4, T::None, floatColor, S | F | W | V},
    {"R16G16Uint", 4, T::Integer, intColor, S | R | M | W | V | TB},
    {"R16G16Sint", 4, T::Integer, intColor, S | R | M | W | V | TB},
    {"R16G16Float", 4, T::None, floatColor, S | F | R | B | M | W | V | TB},
    {"R16G16B16A16Unorm", 8, T::None, floatColor, S | F | R | B | M | W | V},
    {"R16G16B16A16Snorm", 8, T::None, floatColor, S | F | W | V},
    {"R16G16B16A16Uint", 8, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R16G16B16A16Sint", 8, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R16G16B16A16Float", 8, T::None, floatColor, S | F | R | B | M | W | V | TB | U},

    {"R32Uint", 4, T::Integer, intColor | Atomics, S | R | M | W | L | A | V | TB | U},
    {"R32Sint", 4, T::Integer, intColor | Atomics, S | R | M | W | L | A | V | TB | U},
    {"R32Float", 4, T::None, floatColor, S | F | R | B | M | W | L | V | TB | U},
    {"R32G32Uint", 8, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R32G32Sint", 8, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R32G32Float", 8, T::None, floatColor, S | F | R | B | M | W | V | TB | U},
    {"R32G32B32Uint", 12, T::Integer | T::VertexOnly, VertexBuffer, V},
    {"R32G32B32Sint", 12, T::Integer | T::VertexOnly, VertexBuffer, V},
    {"R32G32B32Float", 12, T::VertexOnly, VertexBuffer, V},
    {"R32G32B32A32Uint", 16, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R32G32B32A32Sint", 16, T::Integer, intColor, S | R | M | W | V | TB | U},
    {"R32G32B32A32Float", 16, T::None, floatColor, S | F | R | B | M | W | V | TB | U},

    {"R10G10B10A2Unorm", 4, T::None, floatColor, S | F | R | B | M | W | V | TB},
    {"R10G10B10A2Uint", 4, T::Integer, intColor, S | R | M | W | TB},
    {"R11G11B10Float", 4, T::None, floatColor & ~VertexBuffer, S | F | R | B | M | W | TB},
    {"R9G9B9E5SharedExp", 4, T::None, sampledOnly, S | F},

    {"D16Unorm", 2, T::Depth, depth, S | M | D},
    {"D32Float", 4, T::Depth, depth, S | M | D},
    {"D32FloatS8Uint", 8, T::Depth | T::Stencil, depth, S | M | D},

    {"BC1Unorm", 8, T::Compressed, sampledOnly, S | F},
    {"BC1UnormSrgb", 8, T::Compressed | T::Srgb, sampledOnly, S | F},
    {"BC3Unorm", 16, T::Compressed, sampledOnly, S | F},
    {"BC3UnormSrgb", 16, T::Compressed | T::Srgb, sampledOnly, S | F},
    {"BC4Unorm", 8, T::Compressed, sampledOnly, S | F},
    {"BC4Snorm", 8, T::Compressed, sampledOnly, S | F},
    {"BC5Unorm", 16, T::Compressed, sampledOnly, S | F},
    {"BC5Snorm", 16, T::Compressed, sampledOnly, S | F},
    {"BC6HUfloat16", 16, T::Compressed, sampledOnly, S | F},
    {"BC6HSfloat16", 16, T::Compressed, sampledOnly, S | F},
    {"BC7Unorm", 16, T::Compressed, sampledOnly, S | F},
    {"BC7UnormSrgb", 16, T::Compressed | T::Srgb, sampledOnly, S | F},
};
static_assert(sizeof(formats) / sizeof(formats[0]) == pixelFormatCount);

// The format with the sRGB flag removed, or the format itself.
PixelFormat linearOf(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::R8G8B8A8UnormSrgb:
        return PixelFormat::R8G8B8A8Unorm;
    case PixelFormat::B8G8R8A8UnormSrgb:
        return PixelFormat::B8G8R8A8Unorm;
    case PixelFormat::BC1UnormSrgb:
        return PixelFormat::BC1Unorm;
    case PixelFormat::BC3UnormSrgb:
        return PixelFormat::BC3Unorm;
    case PixelFormat::BC7UnormSrgb:
        return PixelFormat::BC7Unorm;
    default:
        return format;
    }
}

} // namespace

const FormatInfo& formatInfo(PixelFormat format)
{
    size_t index = static_cast<size_t>(format);
    return formats[index < pixelFormatCount ? index : 0];
}

bool formatsCastable(PixelFormat a, PixelFormat b)
{
    if (linearOf(a) == linearOf(b))
    {
        return true;
    }
    const FormatInfo& infoA = formatInfo(a);
    const FormatInfo& infoB = formatInfo(b);
    constexpr FormatTraits excluded = FormatTraits::Depth | FormatTraits::Compressed | FormatTraits::VertexOnly;
    if ((infoA.traits & excluded) != FormatTraits::None || (infoB.traits & excluded) != FormatTraits::None)
    {
        return false; // no block-compressed or depth casts in v1 (ADR 0022)
    }
    return a != PixelFormat::Undefined && b != PixelFormat::Undefined && infoA.blockBytes == infoB.blockBytes;
}

PixelFormat missingGuaranteedSupport(const FormatSupport* support, FormatSupport& missing)
{
    for (size_t i = 1; i < pixelFormatCount; ++i)
    {
        missing = formats[i].guaranteed & ~support[i];
        if (missing != FormatSupport::None)
        {
            return static_cast<PixelFormat>(i);
        }
    }
    missing = FormatSupport::None;
    return PixelFormat::Undefined;
}

bool meetsFormatBaseline(const InstanceCommon& instance, const char* deviceName, const FormatSupport* support)
{
    FormatSupport missing = FormatSupport::None;
    PixelFormat format = missingGuaranteedSupport(support, missing);
    if (format == PixelFormat::Undefined)
    {
        return true;
    }
    char names[192];
    describeFormatSupport(missing, names, sizeof(names));
    logMessage(messageSink(&instance), MessageSeverity::Info, MessageSource::Slag,
               "slag: device \"%s\" skipped: %s lacks guaranteed support (%s)", deviceName, formatInfo(format).name,
               names);
    return false;
}

void describeFormatSupport(FormatSupport support, char* out, size_t outSize)
{
    static constexpr const char* names[] = {
        "Sampled",   "LinearFilter",  "RenderTarget", "Blend",        "Multisample2", "Multisample4", "Multisample8",
        "RWTexture", "RWTextureLoad", "Atomics",      "DepthStencil", "VertexBuffer", "TypedBuffer",  "RWTypedBuffer"};
    size_t used = 0;
    out[0] = '\0';
    for (size_t bit = 0; bit < sizeof(names) / sizeof(names[0]); ++bit)
    {
        if ((static_cast<uint32_t>(support) & (1u << bit)) == 0)
        {
            continue;
        }
        int written = std::snprintf(out + used, outSize - used, "%s%s", used ? ", " : "", names[bit]);
        if (written < 0 || used + static_cast<size_t>(written) >= outSize)
        {
            return;
        }
        used += static_cast<size_t>(written);
    }
}

} // namespace slag
