#pragma once

// PixelFormat to VkFormat, and per-device format support (ADR 0022).

#include "vulkan/VkBackend.h"

namespace slag::vk {

VkFormat toVkFormat(PixelFormat format);

// Fills support[pixelFormatCount] for a physical device.
void queryFormatSupport(const InstanceImpl& instance, VkPhysicalDevice device, FormatSupport* support);

} // namespace slag::vk
