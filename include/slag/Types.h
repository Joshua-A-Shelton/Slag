#pragma once

// Small value types shared by several headers, so object headers don't include
// each other for one enum or struct: CompareOp (samplers, depth-stencil state),
// Viewport and Rect (command buffers), ClearValue (textures, render passes).

#include <cstdint>

namespace slag {

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
