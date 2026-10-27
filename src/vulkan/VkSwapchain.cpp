#include "common/Logging.h"
#include "vulkan/VkBackend.h"
#include "vulkan/VkFormats.h"

#ifdef _WIN32
extern "C" __declspec(dllimport) void* __stdcall GetModuleHandleW(const wchar_t* moduleName);
#endif

namespace slag::vk {

namespace {

GraphicsDeviceImpl& deviceOf(SwapchainCommon* swapchain)
{
    return *static_cast<GraphicsDeviceImpl*>(swapchain->device);
}

VkColorSpaceKHR toColorSpace(ColorSpace colorSpace)
{
    switch (colorSpace)
    {
    case ColorSpace::SrgbNonlinear:
        return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    case ColorSpace::Hdr10:
        return VK_COLOR_SPACE_HDR10_ST2084_EXT;
    case ColorSpace::ScRgbLinear:
        return VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT;
    }
    return VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
}

VkPresentModeKHR toPresentMode(PresentMode mode)
{
    switch (mode)
    {
    case PresentMode::Fifo:
        return VK_PRESENT_MODE_FIFO_KHR;
    case PresentMode::Mailbox:
        return VK_PRESENT_MODE_MAILBOX_KHR;
    case PresentMode::Immediate:
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

bool fromPresentMode(VkPresentModeKHR vkMode, PresentMode& mode)
{
    switch (vkMode)
    {
    case VK_PRESENT_MODE_FIFO_KHR:
        mode = PresentMode::Fifo;
        return true;
    case VK_PRESENT_MODE_MAILBOX_KHR:
        mode = PresentMode::Mailbox;
        return true;
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        mode = PresentMode::Immediate;
        return true;
    default:
        return false; // FIFO_RELAXED and others are cut (ADR 0056)
    }
}

// ADR 0055: no surface object in the API; one per swapchain.
VkResult createSurface(const InstanceImpl& instance, const NativeWindow& window, VkSurfaceKHR& surface)
{
    const InstanceFunctions& fn = instance.fn;
    switch (window.kind)
    {
    case WindowKind::Win32:
    {
        if (!fn.vkCreateWin32SurfaceKHR)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        SlagWin32SurfaceCreateInfo info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
#ifdef _WIN32
        info.hinstance = window.display ? window.display : GetModuleHandleW(nullptr);
#else
        info.hinstance = window.display;
#endif
        info.hwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(window.window));
        return fn.vkCreateWin32SurfaceKHR(instance.instance, &info, nullptr, &surface);
    }
    case WindowKind::Xlib:
    {
        if (!fn.vkCreateXlibSurfaceKHR)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        SlagXlibSurfaceCreateInfo info = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
        info.dpy = window.display;
        info.window = static_cast<unsigned long>(window.window);
        return fn.vkCreateXlibSurfaceKHR(instance.instance, &info, nullptr, &surface);
    }
    case WindowKind::Xcb:
    {
        if (!fn.vkCreateXcbSurfaceKHR)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        SlagXcbSurfaceCreateInfo info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
        info.connection = window.display;
        info.window = static_cast<uint32_t>(window.window);
        return fn.vkCreateXcbSurfaceKHR(instance.instance, &info, nullptr, &surface);
    }
    case WindowKind::Wayland:
    {
        if (!fn.vkCreateWaylandSurfaceKHR)
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
        SlagWaylandSurfaceCreateInfo info = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        info.display = window.display;
        info.surface = reinterpret_cast<void*>(static_cast<uintptr_t>(window.window));
        return fn.vkCreateWaylandSurfaceKHR(instance.instance, &info, nullptr, &surface);
    }
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// The present modes the surface supports and, for `mode`, the ones a
// swapchain can switch between (VK_KHR_surface_maintenance1).
uint32_t compatibleModes(const GraphicsDeviceImpl& device, VkSurfaceKHR surface, PresentMode mode, PresentMode* out)
{
    const InstanceImpl& instance = *static_cast<const InstanceImpl*>(device.instance);
    VkSurfacePresentModeKHR presentMode = {VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_KHR};
    presentMode.presentMode = toPresentMode(mode);
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surfaceInfo.pNext = &presentMode;
    surfaceInfo.surface = surface;
    VkPresentModeKHR modes[8] = {};
    VkSurfacePresentModeCompatibilityKHR compatibility = {VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_KHR};
    compatibility.presentModeCount = 8;
    compatibility.pPresentModes = modes;
    VkSurfaceCapabilities2KHR capabilities = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
    capabilities.pNext = &compatibility;
    if (instance.fn.vkGetPhysicalDeviceSurfaceCapabilities2KHR(device.physicalDevice, &surfaceInfo, &capabilities) !=
        VK_SUCCESS)
    {
        return 0;
    }
    uint32_t count = 0;
    for (uint32_t i = 0; i < compatibility.presentModeCount && i < 8; ++i)
    {
        PresentMode converted;
        if (fromPresentMode(modes[i], converted) && count < 3)
        {
            out[count++] = converted;
        }
    }
    return count;
}

void destroyImages(GraphicsDeviceImpl& device, SwapchainImpl& swapchain)
{
    for (TextureCommon* texture : swapchain.textures)
    {
        destroyObject(device.instance, static_cast<TextureImpl*>(texture)); // the images belong to the swapchain
    }
    swapchain.textures.clear();
}

// Waits for presentation to be done with every semaphore (present fences).
void waitPresents(GraphicsDeviceImpl& device, SwapchainImpl& swapchain)
{
    for (PresentSlot& slot : swapchain.presentSlots)
    {
        if (slot.used)
        {
            device.fn.vkWaitForFences(device.device, 1, &slot.fence, VK_TRUE, UINT64_MAX);
        }
    }
}

// Creates (or recreates, with oldSwapchain) the VkSwapchainKHR and wraps its images.
Status buildSwapchain(GraphicsDeviceImpl& device, SwapchainImpl& swapchain, uint32_t width, uint32_t height,
                      uint32_t imageCount, PixelFormat format, ColorSpace colorSpace, PresentMode mode,
                      const char* debugName)
{
    const InstanceImpl& instance = instanceOf(device);
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surfaceInfo.surface = swapchain.surface;
    VkSurfaceCapabilities2KHR capabilities = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
    VkResult result =
        instance.fn.vkGetPhysicalDeviceSurfaceCapabilities2KHR(device.physicalDevice, &surfaceInfo, &capabilities);
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }
    const VkSurfaceCapabilitiesKHR& caps = capabilities.surfaceCapabilities;

    PresentMode modes[3] = {};
    uint32_t modeCount = compatibleModes(device, swapchain.surface, mode, modes);
    bool supported = false;
    for (uint32_t i = 0; i < modeCount; ++i)
    {
        supported = supported || modes[i] == mode;
    }
    if (!supported)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: the window doesn't support the requested present mode");
        return Status::Unsupported;
    }
    VkPresentModeKHR vkModes[3] = {};
    for (uint32_t i = 0; i < modeCount; ++i)
    {
        vkModes[i] = toPresentMode(modes[i]);
    }

    // 8-bit formats also get their sRGB variant for attachment views (ADR 0056).
    VkFormat viewFormats[2] = {toVkFormat(format), VK_FORMAT_UNDEFINED};
    PixelFormat srgb = format == PixelFormat::B8G8R8A8Unorm   ? PixelFormat::B8G8R8A8UnormSrgb
                       : format == PixelFormat::R8G8B8A8Unorm ? PixelFormat::R8G8B8A8UnormSrgb
                                                              : PixelFormat::Undefined;
    viewFormats[1] = toVkFormat(srgb);
    VkImageFormatListCreateInfo formatList = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
    formatList.viewFormatCount = srgb == PixelFormat::Undefined ? 1 : 2;
    formatList.pViewFormats = viewFormats;
    VkSwapchainPresentModesCreateInfoKHR presentModes = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODES_CREATE_INFO_KHR};
    presentModes.pNext = &formatList;
    presentModes.presentModeCount = modeCount;
    presentModes.pPresentModes = vkModes;

    // The image count limits depend on the present mode (surface_maintenance1):
    // RADV on Wayland needs 4 images for Mailbox but reports 3 without a mode.
    // The swapchain can switch between all compatible modes, so the count must
    // satisfy each of them.
    uint32_t minLimit = caps.minImageCount;
    uint32_t maxLimit = caps.maxImageCount; // 0: no limit
    for (uint32_t i = 0; i < modeCount; ++i)
    {
        VkSurfacePresentModeKHR modeInfo = {VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_KHR};
        modeInfo.presentMode = vkModes[i];
        VkPhysicalDeviceSurfaceInfo2KHR modeSurfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        modeSurfaceInfo.pNext = &modeInfo;
        modeSurfaceInfo.surface = swapchain.surface;
        VkSurfaceCapabilities2KHR modeCapabilities = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
        result = instance.fn.vkGetPhysicalDeviceSurfaceCapabilities2KHR(device.physicalDevice, &modeSurfaceInfo,
                                                                        &modeCapabilities);
        if (result != VK_SUCCESS)
        {
            return toStatus(device, result);
        }
        const VkSurfaceCapabilitiesKHR& modeCaps = modeCapabilities.surfaceCapabilities;
        minLimit = modeCaps.minImageCount > minLimit ? modeCaps.minImageCount : minLimit;
        if (modeCaps.maxImageCount != 0 && (maxLimit == 0 || modeCaps.maxImageCount < maxLimit))
        {
            maxLimit = modeCaps.maxImageCount;
        }
    }
    uint32_t minImages = imageCount < minLimit ? minLimit : imageCount;
    if (maxLimit != 0 && minImages > maxLimit)
    {
        minImages = maxLimit;
    }
    VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.pNext = &presentModes;
    createInfo.flags = srgb == PixelFormat::Undefined ? 0 : VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;
    createInfo.surface = swapchain.surface;
    createInfo.minImageCount = minImages;
    createInfo.imageFormat = toVkFormat(format);
    createInfo.imageColorSpace = toColorSpace(colorSpace);
    createInfo.imageExtent = {width, height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (device.sharingFamilyCount > 1)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = device.sharingFamilyCount;
        createInfo.pQueueFamilyIndices = device.sharingFamilies;
    }
    createInfo.preTransform = caps.currentTransform;
    // Opaque only in v1 (ADR 0056).
    createInfo.compositeAlpha = (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
                                    ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
                                    : VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    createInfo.presentMode = toPresentMode(mode);
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain.swapchain;
    VkSwapchainKHR created = VK_NULL_HANDLE;
    result = device.fn.vkCreateSwapchainKHR(device.device, &createInfo, nullptr, &created);
    if (swapchain.swapchain != VK_NULL_HANDLE)
    {
        // Retired either way; the user made sure the GPU is done with it (ADR 0058).
        waitPresents(device, swapchain);
        destroyImages(device, swapchain);
        device.fn.vkDestroySwapchainKHR(device.device, swapchain.swapchain, nullptr);
        swapchain.swapchain = VK_NULL_HANDLE;
    }
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }
    swapchain.swapchain = created;
    setObjectName(device, VK_OBJECT_TYPE_SWAPCHAIN_KHR, reinterpret_cast<uint64_t>(created), debugName);

    uint32_t count = 0;
    device.fn.vkGetSwapchainImagesKHR(device.device, created, &count, nullptr);
    VkImage images[16] = {};
    count = count < 16 ? count : 16;
    device.fn.vkGetSwapchainImagesKHR(device.device, created, &count, images);
    swapchain.imageAcquireSemaphores.clear();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto* texture = device.instance->allocator.create<TextureImpl>();
        if (!texture || !swapchain.textures.pushBack(texture) ||
            !swapchain.imageAcquireSemaphores.pushBack(VK_NULL_HANDLE))
        {
            device.instance->allocator.destroy(texture);
            return Status::OutOfHostMemory;
        }
        texture->device = &device;
        texture->functions = device.functions;
        texture->image = images[i];
        texture->swapchainImage = true;
        texture->type = TextureType::Texture2D;
        texture->format = format;
        texture->width = width;
        texture->height = height;
        texture->usage = TextureUsage::RenderTarget;
        if (srgb != PixelFormat::Undefined)
        {
            texture->castableFormats[0] = srgb;
            texture->castableCount = 1;
        }
    }
    for (uint32_t i = 0; i < modeCount; ++i)
    {
        swapchain.switchableModes[i] = modes[i];
    }
    swapchain.switchableCount = modeCount;

    // Enough acquire semaphores for every image plus one in flight; present
    // slots for two frames per image.
    while (swapchain.freeAcquireSemaphores.size() < count + 1)
    {
        VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore semaphore = VK_NULL_HANDLE;
        result = device.fn.vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS || !swapchain.freeAcquireSemaphores.pushBack(semaphore))
        {
            return result != VK_SUCCESS ? toStatus(device, result) : Status::OutOfHostMemory;
        }
    }
    while (swapchain.presentSlots.size() < count * 2)
    {
        PresentSlot slot;
        VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        result = device.fn.vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &slot.semaphore);
        if (result == VK_SUCCESS)
        {
            result = device.fn.vkCreateFence(device.device, &fenceInfo, nullptr, &slot.fence);
        }
        if (result != VK_SUCCESS || !swapchain.presentSlots.pushBack(slot))
        {
            return result != VK_SUCCESS ? toStatus(device, result) : Status::OutOfHostMemory;
        }
    }
    return Status::Ok;
}

void destroyImpl(GraphicsDeviceImpl& device, SwapchainImpl* swapchain)
{
    waitPresents(device, *swapchain);
    destroyImages(device, *swapchain);
    for (VkSemaphore semaphore : swapchain->freeAcquireSemaphores)
    {
        device.fn.vkDestroySemaphore(device.device, semaphore, nullptr);
    }
    for (VkSemaphore semaphore : swapchain->imageAcquireSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            device.fn.vkDestroySemaphore(device.device, semaphore, nullptr);
        }
    }
    for (PresentSlot& slot : swapchain->presentSlots)
    {
        if (slot.acquireSemaphore != VK_NULL_HANDLE)
        {
            device.fn.vkDestroySemaphore(device.device, slot.acquireSemaphore, nullptr);
        }
        device.fn.vkDestroySemaphore(device.device, slot.semaphore, nullptr);
        device.fn.vkDestroyFence(device.device, slot.fence, nullptr);
    }
    if (swapchain->swapchain != VK_NULL_HANDLE)
    {
        device.fn.vkDestroySwapchainKHR(device.device, swapchain->swapchain, nullptr);
    }
    if (swapchain->surface != VK_NULL_HANDLE)
    {
        instanceOf(device).fn.vkDestroySurfaceKHR(instanceOf(device).instance, swapchain->surface, nullptr);
    }
    destroyObject(device.instance, swapchain);
}

} // namespace

Result<SwapchainCommon*> createSwapchain(GraphicsDeviceCommon* common, const SwapchainDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    InstanceImpl& instance = instanceOf(device);
    MessageSink sink = messageSink(&instance);
    if (!device.presentation)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createSwapchain: \"%s\" lacks VK_KHR_swapchain, swapchain_maintenance1, or "
                   "swapchain_mutable_format",
                   device.info.name);
        return Status::Unsupported;
    }
    auto* swapchain = instance.allocator.create<SwapchainImpl>();
    if (!swapchain)
    {
        return Status::OutOfHostMemory;
    }
    swapchain->device = &device;
    swapchain->functions = device.functions;
    swapchain->textures = Vector<TextureCommon*>(&instance.allocator);
    swapchain->views = Vector<AttachmentView*>(&instance.allocator);
    swapchain->srgbViews = Vector<AttachmentView*>(&instance.allocator);
    swapchain->freeAcquireSemaphores = Vector<VkSemaphore>(&instance.allocator);
    swapchain->imageAcquireSemaphores = Vector<VkSemaphore>(&instance.allocator);
    swapchain->presentSlots = Vector<PresentSlot>(&instance.allocator);

    VkResult result = createSurface(instance, desc.window, swapchain->surface);
    if (result != VK_SUCCESS)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createSwapchain: can't create a surface for this window (%d)",
                   static_cast<int>(result));
        destroyImpl(device, swapchain);
        return Status::Unsupported;
    }
    // Presentation support is checked here, not when listing (ADR 0013).
    auto* queue = static_cast<CommandQueueImpl*>(desc.queue);
    VkBool32 presentable = VK_FALSE;
    instance.fn.vkGetPhysicalDeviceSurfaceSupportKHR(device.physicalDevice, queue->family, swapchain->surface,
                                                     &presentable);
    if (!presentable)
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createSwapchain: the queue can't present to this window");
        destroyImpl(device, swapchain);
        return Status::Unsupported;
    }
    Status status = buildSwapchain(device, *swapchain, desc.width, desc.height, desc.minImageCount, desc.format,
                                   desc.colorSpace, desc.presentMode, desc.debugName);
    if (status != Status::Ok)
    {
        destroyImpl(device, swapchain);
        return status;
    }
    return static_cast<SwapchainCommon*>(swapchain);
}

void destroySwapchain(GraphicsDeviceCommon* common, SwapchainCommon* object)
{
    destroyImpl(*static_cast<GraphicsDeviceImpl*>(common), static_cast<SwapchainImpl*>(object));
}

Result<void> reconfigureSwapchain(SwapchainCommon* object, const SwapchainReconfigureDesc& desc)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    // Acquire semaphores were all waited by presented frames; return them to the pool.
    for (VkSemaphore& semaphore : swapchain->imageAcquireSemaphores)
    {
        if (semaphore != VK_NULL_HANDLE && swapchain->freeAcquireSemaphores.pushBack(semaphore))
        {
            semaphore = VK_NULL_HANDLE;
        }
    }
    return buildSwapchain(device, *swapchain, desc.width, desc.height, desc.minImageCount, desc.format, desc.colorSpace,
                          desc.presentMode, nullptr);
}

Result<uint32_t> acquireImage(SwapchainCommon* object)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    if (swapchain->freeAcquireSemaphores.empty())
    {
        // Take back the semaphores of presents that have finished.
        for (PresentSlot& slot : swapchain->presentSlots)
        {
            if (slot.acquireSemaphore != VK_NULL_HANDLE &&
                device.fn.vkGetFenceStatus(device.device, slot.fence) == VK_SUCCESS &&
                swapchain->freeAcquireSemaphores.pushBack(slot.acquireSemaphore))
            {
                slot.acquireSemaphore = VK_NULL_HANDLE;
            }
        }
    }
    if (swapchain->freeAcquireSemaphores.empty())
    {
        VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkResult created = device.fn.vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore);
        if (created != VK_SUCCESS)
        {
            return toStatus(device, created);
        }
        if (!swapchain->freeAcquireSemaphores.pushBack(semaphore))
        {
            device.fn.vkDestroySemaphore(device.device, semaphore, nullptr);
            return Status::OutOfHostMemory;
        }
    }
    VkSemaphore semaphore = swapchain->freeAcquireSemaphores[swapchain->freeAcquireSemaphores.size() - 1];
    uint32_t index = 0;
    VkResult result = device.fn.vkAcquireNextImageKHR(device.device, swapchain->swapchain, UINT64_MAX, semaphore,
                                                      VK_NULL_HANDLE, &index);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        return toStatus(device, result);
    }
    swapchain->freeAcquireSemaphores.popBack();
    VkSemaphore& previous = swapchain->imageAcquireSemaphores[index];
    if (previous != VK_NULL_HANDLE)
    {
        (void)swapchain->freeAcquireSemaphores.pushBack(previous); // capacity: it was taken from there
    }
    previous = semaphore;
    return Result<uint32_t>(toStatus(device, result), index);
}

VkSemaphore acquireSemaphore(SwapchainCommon* object)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    return object->acquiredImage < swapchain->imageAcquireSemaphores.size()
               ? swapchain->imageAcquireSemaphores[object->acquiredImage]
               : VK_NULL_HANDLE;
}

Result<void> presentImage(CommandQueueCommon* queueObject, SwapchainCommon* object, PresentMode mode)
{
    auto* queue = static_cast<CommandQueueImpl*>(queueObject);
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    InstanceImpl& instance = instanceOf(device);

    // A free slot: its fence says presentation is done with the semaphore.
    PresentSlot& slot = swapchain->presentSlots[swapchain->nextPresentSlot];
    swapchain->nextPresentSlot =
        (swapchain->nextPresentSlot + 1) % static_cast<uint32_t>(swapchain->presentSlots.size());
    if (slot.used)
    {
        device.fn.vkWaitForFences(device.device, 1, &slot.fence, VK_TRUE, UINT64_MAX);
        device.fn.vkResetFences(device.device, 1, &slot.fence);
    }
    slot.used = true;
    if (slot.acquireSemaphore != VK_NULL_HANDLE)
    {
        // That present has finished (the fence above): its semaphore is free.
        if (!swapchain->freeAcquireSemaphores.pushBack(slot.acquireSemaphore))
        {
            device.fn.vkDestroySemaphore(device.device, slot.acquireSemaphore, nullptr);
        }
        slot.acquireSemaphore = VK_NULL_HANDLE;
    }
    if (object->acquiredImage < swapchain->imageAcquireSemaphores.size())
    {
        slot.acquireSemaphore = swapchain->imageAcquireSemaphores[object->acquiredImage];
        swapchain->imageAcquireSemaphores[object->acquiredImage] = VK_NULL_HANDLE;
    }

    // An empty submit signals the render-done semaphore after all earlier work
    // on the queue (ADR 0057).
    VkSemaphoreSubmitInfo signal = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
    signal.semaphore = slot.semaphore;
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    VkSubmitInfo2 submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal;

    VkSwapchainPresentFenceInfoKHR fence = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_KHR};
    fence.swapchainCount = 1;
    fence.pFences = &slot.fence;
    VkPresentModeKHR vkMode = toPresentMode(mode);
    VkSwapchainPresentModeInfoKHR modeInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_MODE_INFO_KHR};
    modeInfo.pNext = &fence;
    modeInfo.swapchainCount = 1;
    modeInfo.pPresentModes = &vkMode;
    uint64_t presentId = swapchain->presentId + 1;
    VkPresentIdKHR id = {VK_STRUCTURE_TYPE_PRESENT_ID_KHR};
    id.pNext = &modeInfo;
    id.swapchainCount = 1;
    id.pPresentIds = &presentId;
    uint32_t imageIndex = object->acquiredImage;
    VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.pNext = device.capabilities.frameLatency ? static_cast<const void*>(&id) : &modeInfo;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &slot.semaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain->swapchain;
    present.pImageIndices = &imageIndex;

    VkResult result;
    {
        std::unique_lock<std::mutex> lock;
        if (queue->sharedLock)
        {
            lock = std::unique_lock<std::mutex>(*queue->sharedLock);
        }
        result = device.fn.vkQueueSubmit2(queue->queue, 1, &submit, VK_NULL_HANDLE);
        if (result == VK_SUCCESS)
        {
            result = device.fn.vkQueuePresentKHR(queue->queue, &present);
        }
    }
    swapchain->presentId = presentId;
    if (result != VK_SUCCESS)
    {
        return toStatus(device, result);
    }

    // A window that isn't visible reports a zero-size surface (ADR 0058).
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surfaceInfo.surface = swapchain->surface;
    VkSurfaceCapabilities2KHR capabilities = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};
    if (instance.fn.vkGetPhysicalDeviceSurfaceCapabilities2KHR(device.physicalDevice, &surfaceInfo, &capabilities) ==
            VK_SUCCESS &&
        (capabilities.surfaceCapabilities.currentExtent.width == 0 ||
         capabilities.surfaceCapabilities.currentExtent.height == 0))
    {
        return Status::Occluded;
    }
    return {};
}

uint32_t swapchainFormats(const GraphicsDeviceCommon* common, const NativeWindow& window,
                          std::span<SwapchainFormat> formats)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    const InstanceImpl& instance = *static_cast<const InstanceImpl*>(device.instance);
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (createSurface(instance, window, surface) != VK_SUCCESS)
    {
        return 0;
    }
    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
    surfaceInfo.surface = surface;
    VkSurfaceFormat2KHR vkFormats[64];
    for (VkSurfaceFormat2KHR& format : vkFormats)
    {
        format = {VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR};
    }
    uint32_t vkCount = 64;
    instance.fn.vkGetPhysicalDeviceSurfaceFormats2KHR(device.physicalDevice, &surfaceInfo, &vkCount, vkFormats);
    instance.fn.vkDestroySurfaceKHR(instance.instance, surface, nullptr);

    // The pairs Slag knows (ADRs 0056, 0059).
    const SwapchainFormat known[] = {{PixelFormat::B8G8R8A8Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R8G8B8A8Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R10G10B10A2Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R16G16B16A16Float, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R10G10B10A2Unorm, ColorSpace::Hdr10},
                                     {PixelFormat::R16G16B16A16Float, ColorSpace::ScRgbLinear}};
    uint32_t count = 0;
    for (const SwapchainFormat& candidate : known)
    {
        bool found = false;
        for (uint32_t i = 0; i < vkCount && !found; ++i)
        {
            found = vkFormats[i].surfaceFormat.format == toVkFormat(candidate.format) &&
                    vkFormats[i].surfaceFormat.colorSpace == toColorSpace(candidate.colorSpace);
        }
        if (found)
        {
            if (count < formats.size())
            {
                formats[count] = candidate;
            }
            ++count;
        }
    }
    return count;
}

uint32_t presentModes(const GraphicsDeviceCommon* common, const NativeWindow& window, std::span<PresentMode> modes)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    const InstanceImpl& instance = *static_cast<const InstanceImpl*>(device.instance);
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (createSurface(instance, window, surface) != VK_SUCCESS)
    {
        return 0;
    }
    VkPresentModeKHR vkModes[16];
    uint32_t vkCount = 16;
    instance.fn.vkGetPhysicalDeviceSurfacePresentModesKHR(device.physicalDevice, surface, &vkCount, vkModes);
    instance.fn.vkDestroySurfaceKHR(instance.instance, surface, nullptr);
    uint32_t count = 0;
    for (PresentMode mode : {PresentMode::Fifo, PresentMode::Mailbox, PresentMode::Immediate})
    {
        bool found = false;
        for (uint32_t i = 0; i < vkCount; ++i)
        {
            found = found || vkModes[i] == toPresentMode(mode);
        }
        if (found)
        {
            if (count < modes.size())
            {
                modes[count] = mode;
            }
            ++count;
        }
    }
    return count;
}

void setHdrMetadata(SwapchainCommon* object, const HdrMetadata& metadata)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    if (!device.hdrMetadata || !device.fn.vkSetHdrMetadataEXT)
    {
        logMessage(messageSink(device.instance), MessageSeverity::Info, MessageSource::Slag,
                   "slag: Swapchain::setHdrMetadata: VK_EXT_hdr_metadata is unavailable; skipped (ADR 0059)");
        return;
    }
    VkHdrMetadataEXT hdr = {VK_STRUCTURE_TYPE_HDR_METADATA_EXT};
    hdr.displayPrimaryRed = {metadata.redPrimary[0], metadata.redPrimary[1]};
    hdr.displayPrimaryGreen = {metadata.greenPrimary[0], metadata.greenPrimary[1]};
    hdr.displayPrimaryBlue = {metadata.bluePrimary[0], metadata.bluePrimary[1]};
    hdr.whitePoint = {metadata.whitePoint[0], metadata.whitePoint[1]};
    hdr.maxLuminance = metadata.maxMasteringLuminance;
    hdr.minLuminance = metadata.minMasteringLuminance;
    hdr.maxContentLightLevel = metadata.maxContentLightLevel;
    hdr.maxFrameAverageLightLevel = metadata.maxFrameAverageLightLevel;
    device.fn.vkSetHdrMetadataEXT(device.device, 1, &swapchain->swapchain, &hdr);
}

void setMaxFrameLatency(SwapchainCommon* object, uint32_t frames)
{
    static_cast<SwapchainImpl*>(object)->maxFrameLatency = frames;
}

// Waits until no more than maxFrameLatency presents are outstanding
// (VK_KHR_present_wait, ADR 0057).
Result<void> waitForNextFrame(SwapchainCommon* object, uint64_t timeoutNs)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    if (swapchain->presentId < swapchain->maxFrameLatency)
    {
        return {};
    }
    uint64_t target = swapchain->presentId - swapchain->maxFrameLatency + 1;
    VkResult result = device.fn.vkWaitForPresentKHR(device.device, swapchain->swapchain, target, timeoutNs);
    return toStatus(device, result);
}

} // namespace slag::vk
