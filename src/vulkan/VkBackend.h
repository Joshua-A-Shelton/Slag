#pragma once

// Vulkan backend: entry points (one per line in BackendFunctions.inl) and the
// backend's object types.

#include "common/Backend.h"
#include "common/Objects.h"
#include "vulkan/VkLoader.h"

namespace slag {

const BackendInfo* slagBackend_vulkan();

} // namespace slag

namespace slag::vk {

#define SLAG_BACKEND_FN(ReturnType, name, parameters) ReturnType name parameters;
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN

struct InstanceImpl : InstanceCommon
{
    const GlobalFunctions* global = nullptr;
    InstanceFunctions fn;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    bool debugUtils = false;
};

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties properties = {};
    VkPhysicalDeviceDescriptorHeapPropertiesEXT heapProperties = {};
    uint32_t graphicsQueueFamily = 0;
};

} // namespace slag::vk
