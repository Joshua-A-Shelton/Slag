#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

namespace slag::d3d12 {

namespace {

GraphicsDeviceImpl& deviceOf(SwapchainCommon* swapchain)
{
    return *static_cast<GraphicsDeviceImpl*>(swapchain->device);
}

DXGI_COLOR_SPACE_TYPE toColorSpace(ColorSpace colorSpace)
{
    switch (colorSpace)
    {
    case ColorSpace::SrgbNonlinear:
        return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    case ColorSpace::Hdr10:
        return DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    case ColorSpace::ScRgbLinear:
        return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
    }
    return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
}

bool tearingSupported(IDXGIFactory6* factory)
{
    BOOL allowed = FALSE;
    return SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowed, sizeof(allowed))) &&
           allowed;
}

// Every supported mode switches on any present (ADR 0056).
void fillModes(SwapchainImpl& swapchain)
{
    swapchain.switchableModes[0] = PresentMode::Fifo;
    swapchain.switchableModes[1] = PresentMode::Mailbox;
    swapchain.switchableCount = 2;
    if (swapchain.tearing)
    {
        swapchain.switchableModes[swapchain.switchableCount++] = PresentMode::Immediate;
    }
}

void releaseImages(GraphicsDeviceImpl& device, SwapchainImpl& swapchain)
{
    for (TextureCommon* texture : swapchain.textures)
    {
        auto* impl = static_cast<TextureImpl*>(texture);
        release(impl->resource);
        device.instance->allocator.destroy(impl);
    }
    swapchain.textures.clear();
}

Status wrapImages(GraphicsDeviceImpl& device, SwapchainImpl& swapchain, PixelFormat format, uint32_t width,
                  uint32_t height)
{
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    swapchain.swapchain->GetDesc1(&desc);
    PixelFormat srgb = format == PixelFormat::B8G8R8A8Unorm   ? PixelFormat::B8G8R8A8UnormSrgb
                       : format == PixelFormat::R8G8B8A8Unorm ? PixelFormat::R8G8B8A8UnormSrgb
                                                              : PixelFormat::Undefined;
    for (UINT i = 0; i < desc.BufferCount; ++i)
    {
        auto* texture = device.instance->allocator.create<TextureImpl>();
        if (!texture || !swapchain.textures.pushBack(texture))
        {
            device.instance->allocator.destroy(texture);
            return Status::OutOfHostMemory;
        }
        texture->device = &device;
        texture->functions = device.functions;
        HRESULT result = swapchain.swapchain->GetBuffer(i, IID_PPV_ARGS(&texture->resource));
        if (FAILED(result))
        {
            return toStatus(device, result);
        }
        texture->type = TextureType::Texture2D;
        texture->format = format;
        texture->width = width;
        texture->height = height;
        texture->usage = TextureUsage::RenderTarget;
        // Flip-model buffers are UNORM; sRGB views are allowed (ADR 0056).
        if (srgb != PixelFormat::Undefined)
        {
            texture->castableFormats[0] = srgb;
            texture->castableCount = 1;
        }
    }
    return Status::Ok;
}

Status setColorSpace(GraphicsDeviceImpl& device, SwapchainImpl& swapchain, ColorSpace colorSpace)
{
    DXGI_COLOR_SPACE_TYPE type = toColorSpace(colorSpace);
    UINT support = 0;
    if (FAILED(swapchain.swapchain->CheckColorSpaceSupport(type, &support)) ||
        !(support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
    {
        logMessage(messageSink(device.instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: the window doesn't support the requested color space");
        return Status::Unsupported;
    }
    return toStatus(device, swapchain.swapchain->SetColorSpace1(type));
}

} // namespace

Result<SwapchainCommon*> createSwapchain(GraphicsDeviceCommon* common, const SwapchainDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    InstanceImpl& instance = instanceOf(device);
    if (desc.window.kind != WindowKind::Win32)
    {
        logMessage(messageSink(&instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createSwapchain: D3D12 presents to Win32 windows only (ADR 0055)");
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
    swapchain->window = reinterpret_cast<HWND>(static_cast<uintptr_t>(desc.window.window));
    swapchain->tearing = tearingSupported(instance.factory);
    swapchain->flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT |
                       (swapchain->tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);

    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = desc.width;
    swapchainDesc.Height = desc.height;
    swapchainDesc.Format = toDxgiFormat(desc.format);
    swapchainDesc.SampleDesc.Count = 1;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.BufferCount = desc.imageCount;
    swapchainDesc.Scaling = DXGI_SCALING_NONE; // the size must match the window (ADR 0056)
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // opaque only in v1
    swapchainDesc.Flags = swapchain->flags;
    auto* queue = static_cast<CommandQueueImpl*>(desc.queue);
    IDXGISwapChain1* swapchain1 = nullptr;
    HRESULT result = instance.factory->CreateSwapChainForHwnd(queue->queue, swapchain->window, &swapchainDesc, nullptr,
                                                              nullptr, &swapchain1);
    if (SUCCEEDED(result))
    {
        result = swapchain1->QueryInterface(IID_PPV_ARGS(&swapchain->swapchain));
        release(swapchain1);
    }
    if (FAILED(result))
    {
        logMessage(messageSink(&instance), MessageSeverity::Error, MessageSource::Slag,
                   "slag: GraphicsDevice::createSwapchain: CreateSwapChainForHwnd failed (0x%08lx)",
                   static_cast<unsigned long>(result));
        destroySwapchain(&device, swapchain);
        return toStatus(device, result);
    }
    // No exclusive fullscreen (ADR 0059).
    instance.factory->MakeWindowAssociation(swapchain->window, DXGI_MWA_NO_ALT_ENTER);
    swapchain->frameLatencyWaitable = swapchain->swapchain->GetFrameLatencyWaitableObject();

    Status status = setColorSpace(device, *swapchain, desc.colorSpace);
    if (status == Status::Ok)
    {
        status = wrapImages(device, *swapchain, desc.format, desc.width, desc.height);
    }
    if (status != Status::Ok)
    {
        destroySwapchain(&device, swapchain);
        return status;
    }
    fillModes(*swapchain);
    return static_cast<SwapchainCommon*>(swapchain);
}

void destroySwapchain(GraphicsDeviceCommon* common, SwapchainCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    releaseImages(device, *swapchain);
    if (swapchain->frameLatencyWaitable)
    {
        CloseHandle(swapchain->frameLatencyWaitable);
    }
    release(swapchain->swapchain);
    device.instance->allocator.destroy(swapchain);
}

Result<void> reconfigureSwapchain(SwapchainCommon* object, const SwapchainReconfigureDesc& desc)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    // Buffer references go before ResizeBuffers (ADR 0058).
    releaseImages(device, *swapchain);
    HRESULT result = swapchain->swapchain->ResizeBuffers(desc.imageCount, desc.width, desc.height,
                                                         toDxgiFormat(desc.format), swapchain->flags);
    Status status = toStatus(device, result);
    if (status == Status::Ok)
    {
        status = setColorSpace(device, *swapchain, desc.colorSpace);
    }
    // On failure the old buffers are still there; wrap whatever exists.
    PixelFormat format = status == Status::Ok ? desc.format : swapchain->format;
    uint32_t width = status == Status::Ok ? desc.width : swapchain->width;
    uint32_t height = status == Status::Ok ? desc.height : swapchain->height;
    Status wrapped = wrapImages(device, *swapchain, format, width, height);
    return status != Status::Ok ? status : wrapped;
}

Result<uint32_t> acquireImage(SwapchainCommon* object)
{
    return static_cast<SwapchainImpl*>(object)->swapchain->GetCurrentBackBufferIndex();
}

Result<void> presentImage(CommandQueueCommon*, SwapchainCommon* object, PresentMode mode)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    GraphicsDeviceImpl& device = deviceOf(object);
    UINT interval = mode == PresentMode::Fifo ? 1 : 0;
    UINT flags = mode == PresentMode::Immediate && swapchain->tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
    HRESULT result = swapchain->swapchain->Present(interval, flags);
    if (result == DXGI_STATUS_OCCLUDED)
    {
        return Status::Occluded;
    }
    if (FAILED(result))
    {
        return toStatus(device, result);
    }
    // D3D12 scales instead of failing, so compare with the window (ADR 0058).
    RECT client = {};
    if (GetClientRect(swapchain->window, &client))
    {
        uint32_t width = static_cast<uint32_t>(client.right - client.left);
        uint32_t height = static_cast<uint32_t>(client.bottom - client.top);
        if (width == 0 || height == 0)
        {
            return Status::Occluded;
        }
        if (width != swapchain->width || height != swapchain->height)
        {
            return Status::Suboptimal;
        }
    }
    return {};
}

uint32_t swapchainFormats(const GraphicsDeviceCommon* common, const NativeWindow& window,
                          std::span<SwapchainFormat> formats)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    if (window.kind != WindowKind::Win32)
    {
        return 0;
    }
    // HDR color spaces need an HDR output on this adapter (ADR 0059).
    bool hdr = false;
    for (UINT i = 0;; ++i)
    {
        IDXGIOutput* output = nullptr;
        if (device.adapter->EnumOutputs(i, &output) == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }
        IDXGIOutput6* output6 = nullptr;
        if (SUCCEEDED(output->QueryInterface(IID_PPV_ARGS(&output6))))
        {
            DXGI_OUTPUT_DESC1 desc = {};
            hdr = hdr || (SUCCEEDED(output6->GetDesc1(&desc)) &&
                          desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
            output6->Release();
        }
        output->Release();
    }
    const SwapchainFormat known[] = {{PixelFormat::B8G8R8A8Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R8G8B8A8Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R10G10B10A2Unorm, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R16G16B16A16Float, ColorSpace::SrgbNonlinear},
                                     {PixelFormat::R10G10B10A2Unorm, ColorSpace::Hdr10},
                                     {PixelFormat::R16G16B16A16Float, ColorSpace::ScRgbLinear}};
    uint32_t count = 0;
    for (const SwapchainFormat& candidate : known)
    {
        if (candidate.colorSpace != ColorSpace::SrgbNonlinear && !hdr)
        {
            continue;
        }
        if (count < formats.size())
        {
            formats[count] = candidate;
        }
        ++count;
    }
    return count;
}

uint32_t presentModes(const GraphicsDeviceCommon* common, const NativeWindow& window, std::span<PresentMode> modes)
{
    auto& device = *static_cast<const GraphicsDeviceImpl*>(common);
    if (window.kind != WindowKind::Win32)
    {
        return 0;
    }
    const PresentMode all[] = {PresentMode::Fifo, PresentMode::Mailbox, PresentMode::Immediate};
    uint32_t count = tearingSupported(static_cast<const InstanceImpl*>(device.instance)->factory) ? 3 : 2;
    for (uint32_t i = 0; i < count && i < modes.size(); ++i)
    {
        modes[i] = all[i];
    }
    return count;
}

void setHdrMetadata(SwapchainCommon* object, const HdrMetadata& metadata)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    // DXGI's fixed-point units: chromaticity x 50000, minimum luminance x 10000.
    auto chroma = [](float value) {
        return static_cast<UINT16>(value * 50000.0f + 0.5f);
    };
    DXGI_HDR_METADATA_HDR10 hdr = {};
    hdr.RedPrimary[0] = chroma(metadata.redPrimary[0]);
    hdr.RedPrimary[1] = chroma(metadata.redPrimary[1]);
    hdr.GreenPrimary[0] = chroma(metadata.greenPrimary[0]);
    hdr.GreenPrimary[1] = chroma(metadata.greenPrimary[1]);
    hdr.BluePrimary[0] = chroma(metadata.bluePrimary[0]);
    hdr.BluePrimary[1] = chroma(metadata.bluePrimary[1]);
    hdr.WhitePoint[0] = chroma(metadata.whitePoint[0]);
    hdr.WhitePoint[1] = chroma(metadata.whitePoint[1]);
    hdr.MaxMasteringLuminance = static_cast<UINT>(metadata.maxMasteringLuminance + 0.5f);
    hdr.MinMasteringLuminance = static_cast<UINT>(metadata.minMasteringLuminance * 10000.0f + 0.5f);
    hdr.MaxContentLightLevel = static_cast<UINT16>(metadata.maxContentLightLevel + 0.5f);
    hdr.MaxFrameAverageLightLevel = static_cast<UINT16>(metadata.maxFrameAverageLightLevel + 0.5f);
    swapchain->swapchain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(hdr), &hdr);
}

void setMaxFrameLatency(SwapchainCommon* object, uint32_t frames)
{
    static_cast<SwapchainImpl*>(object)->swapchain->SetMaximumFrameLatency(frames);
}

Result<void> waitForNextFrame(SwapchainCommon* object, uint64_t timeoutNs)
{
    auto* swapchain = static_cast<SwapchainImpl*>(object);
    uint64_t milliseconds = timeoutNs / 1'000'000 + (timeoutNs % 1'000'000 != 0 ? 1 : 0);
    DWORD timeout = timeoutNs == UINT64_MAX    ? INFINITE
                    : milliseconds >= INFINITE ? INFINITE - 1
                                               : static_cast<DWORD>(milliseconds);
    DWORD waited = WaitForSingleObjectEx(swapchain->frameLatencyWaitable, timeout, TRUE);
    return waited == WAIT_OBJECT_0 ? Status::Ok : waited == WAIT_TIMEOUT ? Status::Timeout : Status::Unsupported;
}

} // namespace slag::d3d12
