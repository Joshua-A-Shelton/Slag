#pragma once

// Fixed limit constants (ADR 0028, docs/naming.md). Every device that passes
// the baseline check supports at least these.

#include <cstdint>

namespace slag {

constexpr uint32_t kMaxTexture1DSize = 16384;
constexpr uint32_t kMaxTexture2DSize = 16384;
constexpr uint32_t kMaxTexture3DSize = 2048;
constexpr uint32_t kMaxTextureCubeSize = 16384;
constexpr uint32_t kMaxTextureArrayLayers = 2048;
constexpr uint32_t kMaxMipLevels = 15;
constexpr uint32_t kMaxSampleCount = 8;

constexpr uint32_t kMaxConstantBufferSize = 65536;
constexpr uint32_t kConstantBufferAlignment = 256;
constexpr uint32_t kRawBufferAlignment = 256;
constexpr uint32_t kMaxTypedBufferElements = 1u << 27;
constexpr uint32_t kMaxStructuredBufferElements = 1u << 27;
constexpr uint64_t kMaxStorageBufferViewSize = 1ull << 30;

constexpr uint32_t kMaxPushConstantsSize = 128;

constexpr uint32_t kMaxResourceHeapSize = 1000000;
constexpr uint32_t kDefaultResourceHeapSize = 262144;
constexpr uint32_t kMaxSamplerHeapSize = 2048;
constexpr uint32_t kMaxUniqueSamplers = 4096;

constexpr uint32_t kMaxColorTargets = 8;
constexpr uint32_t kMaxViewports = 16;
constexpr uint32_t kMaxVertexAttributes = 32;
constexpr uint32_t kMaxVertexBufferSlots = 32;

constexpr uint32_t kMaxAnisotropy = 16;
constexpr float kMaxSamplerLodBias = 14.0f;

constexpr uint32_t kTextureCopyOffsetAlignment = 512;
constexpr uint32_t kTextureCopyRowPitchAlignment = 256;

constexpr uint32_t kMaxDebugNameLength = 255;

} // namespace slag
