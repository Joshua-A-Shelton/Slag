// Vulkan functions Slag loads itself (ADR 0061). Add a line when Slag starts
// calling a function.
//
// SLAG_VK_GLOBAL_FN(name)    loaded with vkGetInstanceProcAddr(nullptr, ...)
// SLAG_VK_INSTANCE_FN(name)  loaded with vkGetInstanceProcAddr(instance, ...)
// SLAG_VK_DEVICE_FN(name)    loaded with vkGetDeviceProcAddr(device, ...)

#ifndef SLAG_VK_GLOBAL_FN
#define SLAG_VK_GLOBAL_FN(name)
#endif
#ifndef SLAG_VK_INSTANCE_FN
#define SLAG_VK_INSTANCE_FN(name)
#endif
#ifndef SLAG_VK_DEVICE_FN
#define SLAG_VK_DEVICE_FN(name)
#endif

SLAG_VK_GLOBAL_FN(vkEnumerateInstanceVersion)
SLAG_VK_GLOBAL_FN(vkEnumerateInstanceExtensionProperties)
SLAG_VK_GLOBAL_FN(vkEnumerateInstanceLayerProperties)
SLAG_VK_GLOBAL_FN(vkCreateInstance)

SLAG_VK_INSTANCE_FN(vkDestroyInstance)
SLAG_VK_INSTANCE_FN(vkEnumeratePhysicalDevices)
SLAG_VK_INSTANCE_FN(vkGetPhysicalDeviceProperties2)
SLAG_VK_INSTANCE_FN(vkGetPhysicalDeviceFeatures2)
SLAG_VK_INSTANCE_FN(vkGetPhysicalDeviceQueueFamilyProperties2)
SLAG_VK_INSTANCE_FN(vkGetPhysicalDeviceMemoryProperties2)
SLAG_VK_INSTANCE_FN(vkEnumerateDeviceExtensionProperties)
SLAG_VK_INSTANCE_FN(vkCreateDevice)
SLAG_VK_INSTANCE_FN(vkGetDeviceProcAddr)
SLAG_VK_INSTANCE_FN(vkCreateDebugUtilsMessengerEXT)
SLAG_VK_INSTANCE_FN(vkDestroyDebugUtilsMessengerEXT)

#undef SLAG_VK_GLOBAL_FN
#undef SLAG_VK_INSTANCE_FN
#undef SLAG_VK_DEVICE_FN
