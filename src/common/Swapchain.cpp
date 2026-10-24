#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/CommandQueue.h>
#include <slag/GraphicsDevice.h>
#include <slag/Swapchain.h>

namespace slag {

namespace {

SwapchainCommon* common(Swapchain* swapchain)
{
    return static_cast<SwapchainCommon*>(swapchain);
}

const SwapchainCommon* common(const Swapchain* swapchain)
{
    return static_cast<const SwapchainCommon*>(swapchain);
}

// The sRGB view format of an 8-bit swapchain format (ADR 0056).
PixelFormat srgbVariant(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::B8G8R8A8Unorm:
        return PixelFormat::B8G8R8A8UnormSrgb;
    case PixelFormat::R8G8B8A8Unorm:
        return PixelFormat::R8G8B8A8UnormSrgb;
    default:
        return PixelFormat::Undefined;
    }
}

#ifdef SLAG_VALIDATION
// Formats and color spaces swapchains take (ADRs 0056, 0059).
void validateFormat(InstanceCommon* instance, PixelFormat format, ColorSpace colorSpace, const char* function)
{
    bool known = format == PixelFormat::B8G8R8A8Unorm || format == PixelFormat::R8G8B8A8Unorm ||
                 format == PixelFormat::R10G10B10A2Unorm || format == PixelFormat::R16G16B16A16Float;
    SLAG_VALIDATE(instance, known, function, "%s is not a swapchain format", formatInfo(format).name);
    SLAG_VALIDATE(instance, colorSpace != ColorSpace::Hdr10 || format == PixelFormat::R10G10B10A2Unorm, function,
                  "Hdr10 needs R10G10B10A2Unorm");
    SLAG_VALIDATE(instance, colorSpace != ColorSpace::ScRgbLinear || format == PixelFormat::R16G16B16A16Float, function,
                  "ScRgbLinear needs R16G16B16A16Float");
}

void validateSize(InstanceCommon* instance, uint32_t width, uint32_t height, uint32_t imageCount, const char* function)
{
    // Minimized windows have no size: skip rendering instead (ADR 0058).
    SLAG_VALIDATE(instance, width > 0 && height > 0, function, "the size is 0 (a minimized window?)");
    SLAG_VALIDATE(instance, imageCount == 2 || imageCount == 3, function, "imageCount %u is not 2 or 3", imageCount);
}
#endif

void destroyViews(SwapchainCommon* swapchain)
{
    auto* device = static_cast<GraphicsDevice*>(swapchain->device);
    for (AttachmentView* view : swapchain->views)
    {
        device->destroy(view);
    }
    for (AttachmentView* view : swapchain->srgbViews)
    {
        device->destroy(view);
    }
    swapchain->views.clear();
    swapchain->srgbViews.clear();
}

// One view per image, plus the sRGB variant for 8-bit formats.
Status createViews(SwapchainCommon* swapchain)
{
    auto* device = static_cast<GraphicsDevice*>(swapchain->device);
    PixelFormat srgb = srgbVariant(swapchain->format);
    for (TextureCommon* texture : swapchain->textures)
    {
        Result<AttachmentView*> view = device->createAttachmentView({.texture = texture});
        if (!view || !swapchain->views.pushBack(view.value()))
        {
            device->destroy(view ? view.value() : nullptr);
            return view ? Status::OutOfHostMemory : view.status();
        }
        Result<AttachmentView*> srgbView = srgb == PixelFormat::Undefined
                                               ? Result<AttachmentView*>(nullptr)
                                               : device->createAttachmentView({.texture = texture, .format = srgb});
        if (!srgbView || !swapchain->srgbViews.pushBack(srgbView.value()))
        {
            device->destroy(srgbView ? srgbView.value() : nullptr);
            return srgbView ? Status::OutOfHostMemory : srgbView.status();
        }
    }
    return Status::Ok;
}

} // namespace

Result<Swapchain*> GraphicsDevice::createSwapchain(const SwapchainDesc& desc)
{
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    [[maybe_unused]] const char* function = "GraphicsDevice::createSwapchain";
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->instance;
    SLAG_VALIDATE(instance, self->initialized, function, "device \"%s\" is not initialized", self->info.name);
    auto* queue = static_cast<const CommandQueueCommon*>(desc.queue);
    SLAG_VALIDATE(instance, queue && queue->device == self && queue->type == CommandQueueType::Graphics, function,
                  "swapchains present on a graphics queue of this device (ADR 0013)");
    validateSize(instance, desc.width, desc.height, desc.imageCount, function);
    validateFormat(instance, desc.format, desc.colorSpace, function);
#endif
    if (self->lost)
    {
        return Status::DeviceLost;
    }
    Result<SwapchainCommon*> result = SLAG_DISPATCH(self, createSwapchain)(self, desc);
    if (!result)
    {
        return result.status();
    }
    SwapchainCommon* swapchain = result.value();
    swapchain->userData = desc.userData;
    swapchain->queue = static_cast<CommandQueueCommon*>(desc.queue);
    swapchain->window = desc.window;
    swapchain->width = desc.width;
    swapchain->height = desc.height;
    swapchain->format = desc.format;
    swapchain->colorSpace = desc.colorSpace;
    Status status = createViews(swapchain);
    if (status != Status::Ok)
    {
        destroy(swapchain);
        return status;
    }
    return static_cast<Swapchain*>(swapchain);
}

void GraphicsDevice::destroy(Swapchain* swapchain)
{
    if (!swapchain)
    {
        return;
    }
    auto* self = static_cast<GraphicsDeviceCommon*>(this);
    SwapchainCommon* object = common(swapchain);
    SLAG_VALIDATE(self->instance, object->device == self, "GraphicsDevice::destroy",
                  "the swapchain belongs to another device");
    destroyViews(object);
    SLAG_DISPATCH(self, destroySwapchain)(self, object);
}

uint32_t GraphicsDevice::supportedSwapchainFormats(const NativeWindow& window, std::span<SwapchainFormat> formats) const
{
    auto* self = static_cast<const GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::supportedSwapchainFormats",
                  "device \"%s\" is not initialized", self->info.name);
    return SLAG_DISPATCH(self, swapchainFormats)(self, window, formats);
}

uint32_t GraphicsDevice::supportedPresentModes(const NativeWindow& window, std::span<PresentMode> modes) const
{
    auto* self = static_cast<const GraphicsDeviceCommon*>(this);
    SLAG_VALIDATE(self->instance, self->initialized, "GraphicsDevice::supportedPresentModes",
                  "device \"%s\" is not initialized", self->info.name);
    return SLAG_DISPATCH(self, presentModes)(self, window, modes);
}

Result<uint32_t> Swapchain::acquire()
{
    SwapchainCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->acquiredImage == SwapchainCommon::noImage, "Swapchain::acquire",
                  "image %u is acquired but not presented", self->acquiredImage);
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    Result<uint32_t> result = SLAG_DISPATCH(self, acquireImage)(self);
    if (result)
    {
        self->acquiredImage = result.value();
    }
    return result;
}

uint32_t Swapchain::imageCount() const
{
    return static_cast<uint32_t>(common(this)->textures.size());
}

Texture* Swapchain::texture(uint32_t index) const
{
    const SwapchainCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, index < self->textures.size(), "Swapchain::texture",
                  "image %u is out of range (%zu images)", index, self->textures.size());
    return index < self->textures.size() ? self->textures[index] : nullptr;
}

AttachmentView* Swapchain::attachmentView(uint32_t index, bool srgb) const
{
    const SwapchainCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, index < self->views.size(), "Swapchain::attachmentView",
                  "image %u is out of range (%zu images)", index, self->views.size());
    SLAG_VALIDATE(self->device->instance, !srgb || srgbVariant(self->format) != PixelFormat::Undefined,
                  "Swapchain::attachmentView", "%s has no sRGB variant", formatInfo(self->format).name);
    if (index >= self->views.size())
    {
        return nullptr;
    }
    return srgb ? self->srgbViews[index] : self->views[index];
}

std::span<const PresentMode> Swapchain::switchablePresentModes() const
{
    const SwapchainCommon* self = common(this);
    return {self->switchableModes, self->switchableCount};
}

Result<void> Swapchain::reconfigure(const SwapchainReconfigureDesc& desc)
{
    SwapchainCommon* self = common(this);
    [[maybe_unused]] const char* function = "Swapchain::reconfigure";
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE(instance, self->acquiredImage == SwapchainCommon::noImage, function,
                  "image %u is acquired but not presented", self->acquiredImage);
    validateSize(instance, desc.width, desc.height, desc.imageCount, function);
    validateFormat(instance, desc.format, desc.colorSpace, function);
#endif
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    // Views go first: the backend replaces the textures (ADR 0058).
    destroyViews(self);
    Result<void> result = SLAG_DISPATCH(self, reconfigureSwapchain)(self, desc);
    if (result)
    {
        self->width = desc.width;
        self->height = desc.height;
        self->format = desc.format;
        self->colorSpace = desc.colorSpace;
    }
    Status views = createViews(self); // the old images on failure, which stay valid
    return result ? Result<void>(views) : result;
}

void Swapchain::setHdrMetadata(const HdrMetadata& metadata)
{
    SwapchainCommon* self = common(this);
    SLAG_DISPATCH(self, setHdrMetadata)(self, metadata);
}

void Swapchain::setMaxFrameLatency(uint32_t frames)
{
    SwapchainCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->device->capabilities.frameLatency, "Swapchain::setMaxFrameLatency",
                  "frame latency control is not supported on \"%s\"", self->device->info.name);
    SLAG_VALIDATE(self->device->instance, frames >= 1 && frames <= 16, "Swapchain::setMaxFrameLatency",
                  "%u frames is outside 1..16", frames);
    SLAG_DISPATCH(self, setMaxFrameLatency)(self, frames);
}

Result<void> Swapchain::waitForNextFrame(uint64_t timeoutNs)
{
    SwapchainCommon* self = common(this);
    SLAG_VALIDATE(self->device->instance, self->device->capabilities.frameLatency, "Swapchain::waitForNextFrame",
                  "frame latency control is not supported on \"%s\"", self->device->info.name);
    return SLAG_DISPATCH(self, waitForNextFrame)(self, timeoutNs);
}

uint64_t Swapchain::userData() const
{
    return common(this)->userData;
}

void Swapchain::setUserData(uint64_t value)
{
    common(this)->userData = value;
}

Result<void> CommandQueue::present(Swapchain* swapchain, PresentMode mode)
{
    auto* self = static_cast<CommandQueueCommon*>(this);
    SwapchainCommon* object = common(swapchain);
#ifdef SLAG_VALIDATION
    InstanceCommon* instance = self->device->instance;
    SLAG_VALIDATE(instance, object && object->queue == self, "CommandQueue::present",
                  "the swapchain presents on another queue (ADR 0057)");
    SLAG_VALIDATE(instance, object && object->acquiredImage != SwapchainCommon::noImage, "CommandQueue::present",
                  "no image is acquired");
    bool switchable = false;
    for (uint32_t i = 0; object && i < object->switchableCount; ++i)
    {
        switchable = switchable || object->switchableModes[i] == mode;
    }
    SLAG_VALIDATE(instance, switchable, "CommandQueue::present",
                  "the present mode isn't switchable on this swapchain; reconfigure to change it (ADR 0056)");
#endif
    if (self->device->lost)
    {
        return Status::DeviceLost;
    }
    Result<void> result = SLAG_DISPATCH(self, presentImage)(self, object, mode);
    object->acquiredImage = SwapchainCommon::noImage;
    return result;
}

} // namespace slag
