#pragma once

// Slag's own Vulkan loader (ADR 0061): loads the system loader library at
// runtime and fills per-instance and per-device function tables, so Slag has
// no link-time dependency on vulkan-1.

#include <vulkan/vulkan.h>

// Platform surface creation without X11, Wayland, or Windows headers: these
// mirror VkWin32/Xlib/Xcb/WaylandSurfaceCreateInfoKHR with plain types
// (handles are pointers or integers of the same size).
struct SlagWin32SurfaceCreateInfo
{
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* hinstance;
    void* hwnd;
};

struct SlagXlibSurfaceCreateInfo
{
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* dpy;
    unsigned long window; // XID
};

struct SlagXcbSurfaceCreateInfo
{
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* connection;
    uint32_t window; // xcb_window_t
};

struct SlagWaylandSurfaceCreateInfo
{
    VkStructureType sType;
    const void* pNext;
    VkFlags flags;
    void* display;
    void* surface;
};

typedef VkResult(VKAPI_PTR* PFN_vkCreateWin32SurfaceKHR)(VkInstance, const SlagWin32SurfaceCreateInfo*,
                                                         const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef VkResult(VKAPI_PTR* PFN_vkCreateXlibSurfaceKHR)(VkInstance, const SlagXlibSurfaceCreateInfo*,
                                                        const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef VkResult(VKAPI_PTR* PFN_vkCreateXcbSurfaceKHR)(VkInstance, const SlagXcbSurfaceCreateInfo*,
                                                       const VkAllocationCallbacks*, VkSurfaceKHR*);
typedef VkResult(VKAPI_PTR* PFN_vkCreateWaylandSurfaceKHR)(VkInstance, const SlagWaylandSurfaceCreateInfo*,
                                                           const VkAllocationCallbacks*, VkSurfaceKHR*);

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
