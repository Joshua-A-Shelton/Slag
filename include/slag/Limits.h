#pragma once

// Fixed limit constants (ADR 0028, docs/naming.md). Every device that passes
// the baseline check supports at least these.

#include <cstdint>

namespace slag {

namespace limits {

constexpr uint32_t maxTexture1DSize = 16384;
constexpr uint32_t maxTexture2DSize = 16384;
constexpr uint32_t maxTexture3DSize = 2048;
constexpr uint32_t maxTextureCubeSize = 16384;
constexpr uint32_t maxTextureArrayLayers = 2048;
constexpr uint32_t maxMipLevels = 15;
constexpr uint32_t maxSampleCount = 8;

constexpr uint32_t maxConstantBufferSize = 65536;
constexpr uint32_t maxTypedBufferElements = 1u << 27;
constexpr uint32_t maxStructuredBufferElements = 1u << 27;
constexpr uint64_t maxStorageBufferViewSize = 1ull << 30;

constexpr uint32_t maxPushConstantsSize = 128;

constexpr uint32_t maxResourceHeapSize = 1000000;
constexpr uint32_t maxSamplerHeapSize = 2048;
constexpr uint32_t maxUniqueSamplers = 4096;

constexpr uint32_t maxColorTargets = 8;
constexpr uint32_t maxViewports = 16;
constexpr uint32_t maxVertexAttributes = 32;
constexpr uint32_t maxVertexBufferSlots = 32;

constexpr uint32_t maxAnisotropy = 16;
constexpr float maxSamplerLodBias = 14.0f;

constexpr uint32_t maxDebugNameLength = 255;

} // namespace limits

// Required alignments, in bytes.
namespace alignment {

constexpr uint32_t constantBuffer = 256;
constexpr uint32_t rawBuffer = 256;
constexpr uint32_t textureCopyOffset = 512;
constexpr uint32_t textureCopyRowPitch = 256;

} // namespace alignment

namespace defaults {

constexpr uint32_t resourceHeapSize = 262144;

} // namespace defaults

} // namespace slag
