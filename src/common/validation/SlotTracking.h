#pragma once

// Descriptor-slot tracking (validation builds, ADRs 0031, 0062): which buffer
// or texture each resource slot holds, and how it was written.

#include "common/Objects.h"

#ifdef SLAG_VALIDATION

namespace slag {

void trackBufferSlot(GraphicsDeviceCommon* device, uint32_t index, const BufferDescriptorInfo& info);
void trackTextureSlot(GraphicsDeviceCommon* device, uint32_t index, const TextureDescriptorInfo& info);
// clearResourceDescriptor: the slot is poisoned (D3D12) and holds nothing.
void clearSlot(GraphicsDeviceCommon* device, uint32_t index);
// A buffer or texture is being destroyed: its slots are poisoned (D3D12),
// with one Info message.
void releaseSlots(GraphicsDeviceCommon* device, BufferCommon* buffer);
void releaseSlots(GraphicsDeviceCommon* device, TextureCommon* texture);

} // namespace slag

#endif
