#pragma once

// Slag's own Vulkan loader (ADR 0061): loads the system loader library at
// runtime and fills per-instance and per-device function tables, so Slag has
// no link-time dependency on vulkan-1.

#include <vulkan/vulkan.h>

namespace slag::vk {

struct GlobalFunctions
{
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
#define SLAG_VK_GLOBAL_FN(name) PFN_##name name = nullptr;
#include "vulkan/VkFunctions.inl"
};

struct InstanceFunctions
{
#define SLAG_VK_INSTANCE_FN(name) PFN_##name name = nullptr;
#include "vulkan/VkFunctions.inl"
};

struct DeviceFunctions
{
#define SLAG_VK_DEVICE_FN(name) PFN_##name name = nullptr;
#include "vulkan/VkFunctions.inl"
};

// Loads the Vulkan loader library once per process. Returns null if it isn't
// installed. Thread-safe.
const GlobalFunctions* loadGlobalFunctions();

void loadInstanceFunctions(const GlobalFunctions& global, VkInstance instance, InstanceFunctions& functions);
void loadDeviceFunctions(const InstanceFunctions& instanceFunctions, VkDevice device, DeviceFunctions& functions);

} // namespace slag::vk
