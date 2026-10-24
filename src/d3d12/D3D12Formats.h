#pragma once

// PixelFormat to DXGI_FORMAT, and per-device format support (ADR 0022).

#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

DXGI_FORMAT toDxgiFormat(PixelFormat format);

// Fills support[pixelFormatCount] for a device.
void queryFormatSupport(ID3D12Device* device, FormatSupport* support);

} // namespace slag::d3d12
