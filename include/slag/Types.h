#pragma once

// Small value types shared by several headers, so object headers don't include
// each other for one enum or struct: CompareOp (samplers, depth-stencil state),
// Viewport and Rect (command buffers), ClearValue (textures, render passes).

#include <cstdint>

namespace slag {

// Depth, stencil, and sampler comparisons.
enum class CompareOp : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
};

// Render area coordinates in pixels, origin top-left on both backends.
struct Viewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Rect
{
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

// A render target's clear color, or a depth/stencil target's clear depth and
// stencil (ADR 0046). Also the optional optimized clear value of a texture
// (ADR 0021).
struct ClearValue
{
    float color[4] = {};
    float depth = 0.0f;
    uint8_t stencil = 0;
};

} // namespace slag
