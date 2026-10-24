// Validation of swapchains and presentation.

#include "ValidationFixture.h"

using namespace slag;

namespace {

Swapchain* swapchain(ValidationFixture& f, SwapchainDesc desc = {})
{
    desc.queue = desc.queue ? desc.queue : f.device->commandQueue(CommandQueueType::Graphics);
    desc.width = desc.width ? desc.width : 64;
    desc.height = desc.height ? desc.height : 64;
    return f.device->createSwapchain(desc).value();
}

} // namespace

TEST_CASE("swapchain creation and reconfiguration")
{
    ValidationFixture f;
    auto created = [&](SwapchainDesc desc) {
        f.device->destroy(swapchain(f, desc));
    };
    CHECK_VALIDATION(f, "is not a swapchain format", created({.format = PixelFormat::R32Float}));
    CHECK_VALIDATION(f, "Hdr10 needs R10G10B10A2Unorm", created({.colorSpace = ColorSpace::Hdr10}));
    CHECK_VALIDATION(f, "ScRgbLinear needs R16G16B16A16Float", created({.colorSpace = ColorSpace::ScRgbLinear}));
    CHECK_VALIDATION(f, "minImageCount 4 is not 2 or 3", created({.minImageCount = 4}));
    CHECK_VALIDATION(f, "swapchains present on a graphics queue",
                     created({.queue = f.device->commandQueue(CommandQueueType::Compute)}));
    CHECK_VALIDATION(f, "the size is 0",
                     (void)f.device->createSwapchain({.queue = f.device->commandQueue(CommandQueueType::Graphics)}));

    Swapchain* chain = swapchain(f);
    CHECK_VALIDATION(f, "image 3 is out of range", (void)chain->texture(3));
    CHECK_VALIDATION(f, "image 3 is out of range", (void)chain->attachmentView(3));
    CHECK_VALIDATION(f, "image 3 is out of range", (void)chain->srgbAttachmentView(3));
    REQUIRE(static_cast<bool>(chain->acquire()));
    CHECK_VALIDATION(f, "image 0 is acquired but not presented", (void)chain->acquire());
    CHECK_VALIDATION(f, "image 0 is acquired but not presented", (void)chain->reconfigure({.width = 32, .height = 32}));
    CommandQueue* graphics = f.device->commandQueue(CommandQueueType::Graphics);
    (void)graphics->present(chain, PresentMode::Fifo);
    CHECK_VALIDATION(f, "no image is acquired", (void)graphics->present(chain, PresentMode::Fifo));
    (void)chain->acquire();
    f.common->capabilities.frameLatency = false;
    CHECK_VALIDATIONS(f, 2, "frame latency control is not supported", chain->setMaxFrameLatency(0)); // and 0 frames
    CHECK_VALIDATION(f, "frame latency control is not supported", (void)chain->waitForNextFrame(0));
    f.common->capabilities.frameLatency = true;
    CHECK_VALIDATION(f, "0 frames is outside 1..16", chain->setMaxFrameLatency(0));
    SwapchainCommon* common = static_cast<SwapchainCommon*>(chain);
    common->switchableCount = 1; // Fifo only
    CHECK_VALIDATION(f, "the present mode isn't switchable", (void)graphics->present(chain, PresentMode::Immediate));
    (void)chain->acquire();
    f.device->destroy(chain);

    Swapchain* wide = swapchain(f, {.format = PixelFormat::R16G16B16A16Float, .colorSpace = ColorSpace::ScRgbLinear});
    CHECK_VALIDATION(f, "has no sRGB variant", (void)wide->srgbAttachmentView(0));
    f.device->destroy(wide);

    ValidationFixture other;
    Swapchain* foreign = swapchain(other);
    CHECK_VALIDATION(f, "the swapchain belongs to another device", f.device->destroy(foreign));
    CHECK_VALIDATIONS(f, 2, "presents on another queue", (void)graphics->present(foreign, PresentMode::Fifo));
    other.device->destroy(foreign);

    ValidationFixture uninitialized(false);
    CHECK_VALIDATIONS(uninitialized, 2, "is not initialized",
                      (void)uninitialized.device->createSwapchain({.width = 64, .height = 64})); // and no queue
    NativeWindow window;
    CHECK_VALIDATION(uninitialized, "is not initialized",
                     (void)uninitialized.device->supportedSwapchainFormats(window, {}));
    CHECK_VALIDATION(uninitialized, "is not initialized",
                     (void)uninitialized.device->supportedPresentModes(window, {}));
}

TEST_CASE("destroyed swapchains")
{
    ValidationFixture f;
    Swapchain* chain = swapchain(f);
    f.device->destroy(chain);
    CommandQueue* graphics = f.device->commandQueue(CommandQueueType::Graphics);
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->acquire());
    // acquire() above went ahead, so an image counts as acquired.
    CHECK_VALIDATIONS(f, 2, "use of a destroyed Swapchain", (void)chain->reconfigure({.width = 64, .height = 64}));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->imageCount());
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->texture(0));
    CHECK_VALIDATIONS(f, 2, "use of a destroyed Swapchain", (void)chain->attachmentView(0));     // its views are gone
    CHECK_VALIDATIONS(f, 2, "use of a destroyed Swapchain", (void)chain->srgbAttachmentView(0)); // its views are gone
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->switchablePresentModes());
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", chain->setHdrMetadata({}));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", chain->setMaxFrameLatency(2));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->waitForNextFrame(0));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)chain->userData());
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", chain->setUserData(1));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)graphics->present(chain, PresentMode::Fifo));
    Swapchain* acquired[] = {chain};
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", (void)graphics->submit({.acquiredSwapchains = acquired}));
    CHECK_VALIDATION(f, "use of a destroyed Swapchain", f.device->destroy(chain));
}
