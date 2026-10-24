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
SLAG_VK_INSTANCE_FN(vkSetDebugUtilsObjectNameEXT)

SLAG_VK_DEVICE_FN(vkDestroyDevice)
SLAG_VK_DEVICE_FN(vkGetDeviceQueue)
SLAG_VK_DEVICE_FN(vkDeviceWaitIdle)
SLAG_VK_DEVICE_FN(vkQueueWaitIdle)
SLAG_VK_DEVICE_FN(vkCreateSemaphore)
SLAG_VK_DEVICE_FN(vkDestroySemaphore)
SLAG_VK_DEVICE_FN(vkGetSemaphoreCounterValue)
SLAG_VK_DEVICE_FN(vkSignalSemaphore)
SLAG_VK_DEVICE_FN(vkWaitSemaphores)
SLAG_VK_DEVICE_FN(vkCreateBuffer)
SLAG_VK_DEVICE_FN(vkDestroyBuffer)
SLAG_VK_DEVICE_FN(vkGetBufferMemoryRequirements)
SLAG_VK_DEVICE_FN(vkAllocateMemory)
SLAG_VK_DEVICE_FN(vkFreeMemory)
SLAG_VK_DEVICE_FN(vkBindBufferMemory)
SLAG_VK_DEVICE_FN(vkMapMemory)
SLAG_VK_DEVICE_FN(vkUnmapMemory)
SLAG_VK_DEVICE_FN(vkGetBufferDeviceAddress)
SLAG_VK_DEVICE_FN(vkWriteResourceDescriptorsEXT)
SLAG_VK_DEVICE_FN(vkWriteSamplerDescriptorsEXT)
SLAG_VK_DEVICE_FN(vkCreateCommandPool)
SLAG_VK_DEVICE_FN(vkDestroyCommandPool)
SLAG_VK_DEVICE_FN(vkResetCommandPool)
SLAG_VK_DEVICE_FN(vkAllocateCommandBuffers)
SLAG_VK_DEVICE_FN(vkBeginCommandBuffer)
SLAG_VK_DEVICE_FN(vkEndCommandBuffer)
SLAG_VK_DEVICE_FN(vkQueueSubmit2)
SLAG_VK_DEVICE_FN(vkCreateComputePipelines)
SLAG_VK_DEVICE_FN(vkDestroyPipeline)
SLAG_VK_DEVICE_FN(vkCmdBindPipeline)
SLAG_VK_DEVICE_FN(vkCmdBindResourceHeapEXT)
SLAG_VK_DEVICE_FN(vkCmdBindSamplerHeapEXT)
SLAG_VK_DEVICE_FN(vkCmdPushDataEXT)
SLAG_VK_DEVICE_FN(vkCmdDispatch)
SLAG_VK_DEVICE_FN(vkCmdPipelineBarrier2)
SLAG_VK_DEVICE_FN(vkCmdCopyBuffer)

#undef SLAG_VK_GLOBAL_FN
#undef SLAG_VK_INSTANCE_FN
#undef SLAG_VK_DEVICE_FN
