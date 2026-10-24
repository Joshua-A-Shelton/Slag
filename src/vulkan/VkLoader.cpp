#include "vulkan/VkLoader.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace slag::vk {

namespace {

// The library stays loaded for the life of the process.
PFN_vkGetInstanceProcAddr loadLibrary()
{
#ifdef _WIN32
    HMODULE library = LoadLibraryA("vulkan-1.dll");
    if (!library)
    {
        return nullptr;
    }
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(library, "vkGetInstanceProcAddr"));
#else
    void* library = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!library)
    {
        library = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    }
    if (!library)
    {
        return nullptr;
    }
    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(library, "vkGetInstanceProcAddr"));
#endif
}

GlobalFunctions loadGlobals()
{
    GlobalFunctions functions;
    functions.vkGetInstanceProcAddr = loadLibrary();
    if (!functions.vkGetInstanceProcAddr)
    {
        return functions;
    }
#define SLAG_VK_GLOBAL_FN(name)                                                                                        \
    functions.name = reinterpret_cast<PFN_##name>(functions.vkGetInstanceProcAddr(nullptr, #name));
#include "vulkan/VkFunctions.inl"
    return functions;
}

} // namespace

const GlobalFunctions* loadGlobalFunctions()
{
    static const GlobalFunctions functions = loadGlobals();
    if (!functions.vkGetInstanceProcAddr || !functions.vkCreateInstance)
    {
        return nullptr;
    }
    return &functions;
}

void loadInstanceFunctions(const GlobalFunctions& global, VkInstance instance, InstanceFunctions& functions)
{
#define SLAG_VK_INSTANCE_FN(name)                                                                                      \
    functions.name = reinterpret_cast<PFN_##name>(global.vkGetInstanceProcAddr(instance, #name));
#include "vulkan/VkFunctions.inl"
}

void loadDeviceFunctions(const InstanceFunctions& instanceFunctions, VkDevice device, DeviceFunctions& functions)
{
#define SLAG_VK_DEVICE_FN(name)                                                                                        \
    functions.name = reinterpret_cast<PFN_##name>(instanceFunctions.vkGetDeviceProcAddr(device, #name));
#include "vulkan/VkFunctions.inl"
    (void)instanceFunctions;
    (void)device;
    (void)functions;
}

} // namespace slag::vk
