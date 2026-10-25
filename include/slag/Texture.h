#pragma once

// Texture and the per-type texture Descs, including castable formats (ADRs 0021, 0022).

#include <slag/Flags.h>
#include <slag/MemoryBlock.h>
#include <slag/PixelFormat.h>
#include <slag/Types.h>

#include <cstdint>
#include <span>

namespace slag {

// Every texture can be sampled and copied; these add target and shader-write
// use. RenderTarget and DepthStencil are mutually exclusive.
enum class TextureUsage : uint32_t
{
    None = 0,
    RenderTarget = 1u << 0,
    DepthStencil = 1u << 1,
    ShaderWrite = 1u << 2,
};
SLAG_FLAGS(TextureUsage)

// Fields every texture Desc has, in each one (ADR 0021):
//   format, mipCount (explicit, at least 1), usage, castableFormats (views
//   and copies may use these instead of `format`; ADR 0022), clearValue
//   (optional; only with RenderTarget or DepthStencil; D3D12 uses it, Vulkan
//   ignores it), placement (GpuOnly memory only), debugName, userData.
// Textures start in the Undefined layout with undefined contents.

struct Texture1DDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t mipCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

struct Texture1DArrayDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t layerCount = 1;
    uint32_t mipCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

struct Texture2DDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t mipCount = 1;
    uint32_t sampleCount = 1; // 1, 2, 4, 8; above 1 only with RenderTarget or DepthStencil, and one mip
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

struct Texture2DArrayDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t layerCount = 1;
    uint32_t mipCount = 1;
    uint32_t sampleCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

struct Texture3DDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t mipCount = 1;
    TextureUsage usage = TextureUsage::None; // no DepthStencil
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

// Six square faces; layer i is face i (+X, -X, +Y, -Y, +Z, -Z).
struct TextureCubeDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t size = 1;
    uint32_t mipCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

// cubeCount cubes; layer 6 * cube + face.
struct TextureCubeArrayDesc
{
    PixelFormat format = PixelFormat::Undefined;
    uint32_t size = 1;
    uint32_t cubeCount = 1;
    uint32_t mipCount = 1;
    TextureUsage usage = TextureUsage::None;
    std::span<const PixelFormat> castableFormats;
    const ClearValue* clearValue = nullptr;
    Placement placement;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class Texture
{
public:
    // What the texture was created with. Cubes count six layers per cube;
    // 3D textures have one layer and `depth` slices; 1D textures a height of
    // one. Thread-safe.
    PixelFormat format() const;
    uint32_t width() const;
    uint32_t height() const;
    uint32_t depth() const;
    uint32_t layerCount() const;
    uint32_t mipCount() const;
    uint32_t sampleCount() const;
    TextureUsage usage() const;

    uint64_t userData() const;
    void setUserData(uint64_t value);

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

protected:
    Texture() = default;
    ~Texture() = default;
};

} // namespace slag
