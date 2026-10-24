#include "common/Logging.h"
#include "d3d12/D3D12Backend.h"
#include "d3d12/D3D12Formats.h"

#include <cstdio>
#include <cstring>

namespace slag::d3d12 {

namespace {

// Returns why a device doesn't meet the baseline (ADR 0012), or null.
const char* baselineFailure(ID3D12Device* device)
{
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {D3D_SHADER_MODEL_6_8};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))) ||
        shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_8)
    {
        return "Shader Model 6.8 is not supported";
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
    {
        return "D3D12_OPTIONS can't be queried";
    }
    if (options.ResourceBindingTier < D3D12_RESOURCE_BINDING_TIER_3)
    {
        return "resource binding tier 3 is not supported";
    }
    if (options.ResourceHeapTier < D3D12_RESOURCE_HEAP_TIER_2)
    {
        return "resource heap tier 2 is not supported";
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))))
    {
        return "D3D12_OPTIONS12 can't be queried";
    }
    if (!options12.EnhancedBarriersSupported)
    {
        return "Enhanced Barriers are not supported";
    }
    if (!options12.RelaxedFormatCastingSupported)
    {
        return "relaxed format casting is not supported";
    }
    return nullptr;
}

void fillInfo(IDXGIAdapter4* adapter, const DXGI_ADAPTER_DESC3& desc, ID3D12Device* device, GraphicsDeviceInfo& info)
{
    toUtf8(desc.Description, info.name, sizeof(info.name));
    LARGE_INTEGER umdVersion = {};
    if (SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umdVersion)))
    {
        std::snprintf(
            info.driverVersion, sizeof(info.driverVersion), "%u.%u.%u.%u",
            static_cast<unsigned>(HIWORD(umdVersion.HighPart)), static_cast<unsigned>(LOWORD(umdVersion.HighPart)),
            static_cast<unsigned>(HIWORD(umdVersion.LowPart)), static_cast<unsigned>(LOWORD(umdVersion.LowPart)));
    }
    info.vendorId = desc.VendorId;
    info.deviceId = desc.DeviceId;
    info.dedicatedMemoryBytes = desc.DedicatedVideoMemory;
    info.sharedMemoryBytes = desc.SharedSystemMemory;
    static_assert(sizeof(info.luid) == sizeof(desc.AdapterLuid));
    std::memcpy(info.luid, &desc.AdapterLuid, sizeof(info.luid));

    if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
    {
        info.deviceType = GraphicsDeviceType::Software;
        return;
    }
    D3D12_FEATURE_DATA_ARCHITECTURE1 architecture = {};
    bool uma =
        SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &architecture, sizeof(architecture))) &&
        architecture.UMA;
    info.deviceType = uma ? GraphicsDeviceType::Integrated : GraphicsDeviceType::Discrete;
}

// Creates a device per adapter to check the baseline, then releases it (ADR 0012).
Status listDevices(InstanceImpl& instance, const InstanceDesc& desc)
{
    // DXGI orders adapters by the preference; software adapters go last (ADR 0008).
    DXGI_GPU_PREFERENCE preference = desc.devicePreference == DevicePreference::LowPower
                                         ? DXGI_GPU_PREFERENCE_MINIMUM_POWER
                                         : DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
    for (int pass = 0; pass < 2; ++pass)
    {
        bool softwarePass = pass == 1;
        for (UINT i = 0;; ++i)
        {
            IDXGIAdapter4* adapter = nullptr;
            if (instance.factory->EnumAdapterByGpuPreference(i, preference, IID_PPV_ARGS(&adapter)) ==
                DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            if (!adapter)
            {
                continue;
            }
            DXGI_ADAPTER_DESC3 adapterDesc = {};
            adapter->GetDesc3(&adapterDesc);
            bool software = (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) != 0;
            if (software != softwarePass)
            {
                release(adapter);
                continue;
            }
            char name[256];
            toUtf8(adapterDesc.Description, name, sizeof(name));
            if (software && !desc.allowSoftwareDevices)
            {
                logMessage(messageSink(&instance), MessageSeverity::Info, MessageSource::Slag,
                           "slag: device \"%s\" skipped: software device (allowSoftwareDevices is off)", name);
                release(adapter);
                continue;
            }

            ID3D12Device* device = nullptr;
            const char* failure = nullptr;
            if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device))))
            {
                failure = "feature level 12_0 is not supported";
            }
            else
            {
                failure = baselineFailure(device);
            }
            if (failure)
            {
                logMessage(messageSink(&instance), MessageSeverity::Info, MessageSource::Slag,
                           "slag: device \"%s\" skipped: %s", name, failure);
                release(device);
                release(adapter);
                continue;
            }
            FormatSupport formatSupport[pixelFormatCount];
            queryFormatSupport(device, formatSupport);
            if (!meetsFormatBaseline(instance, name, formatSupport))
            {
                release(device);
                release(adapter);
                continue;
            }

            auto* impl = instance.allocator.create<GraphicsDeviceImpl>();
            if (!impl || !instance.devices.pushBack(impl))
            {
                instance.allocator.destroy(impl);
                release(device);
                release(adapter);
                return Status::OutOfHostMemory;
            }
            impl->instance = &instance;
            impl->functions = instance.functions;
            impl->adapter = adapter;
            for (Vector<CommandQueue*>& queues : impl->queues)
            {
                queues = Vector<CommandQueue*>(&instance.allocator);
            }
            fillInfo(adapter, adapterDesc, device, impl->info);
            for (size_t format = 0; format < pixelFormatCount; ++format)
            {
                impl->formatSupport[format] = formatSupport[format];
            }
            D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
            impl->gpuUploadHeap =
                SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16))) &&
                options16.GPUUploadHeapSupported;
            release(device);
        }
    }
    return Status::Ok;
}

void destroyImpl(InstanceImpl* instance)
{
    for (GraphicsDevice* device : instance->devices)
    {
        auto* impl = static_cast<GraphicsDeviceImpl*>(device);
        if (impl->initialized)
        {
            shutdownDevice(impl); // the user should have shut it down; don't leak the device
        }
        release(impl->adapter);
        instance->allocator.destroy(impl);
    }
    instance->devices.clear();
    release(instance->factory);
    HostAllocator allocator = instance->allocator;
    allocator.destroy(instance);
}

} // namespace

void toUtf8(const wchar_t* text, char* out, size_t outSize)
{
    int written = WideCharToMultiByte(CP_UTF8, 0, text, -1, out, static_cast<int>(outSize), nullptr, nullptr);
    if (written <= 0)
    {
        out[0] = '\0'; // too long or invalid: WideCharToMultiByte writes nothing usable
    }
}

Result<InstanceCommon*> createInstance(const InstanceDesc& desc, const HostAllocator& allocator)
{
    MessageSink sink = messageSink(desc);
    auto* instance = allocator.create<InstanceImpl>();
    if (!instance)
    {
        return Status::OutOfHostMemory;
    }
    initInstanceCommon(*instance, desc, *slagBackend_d3d12(), allocator);

    // Backend validation (ADR 0010). The debug layer is process-wide and stays
    // on once enabled; GPU-based validation needs it.
    bool wantDebugLayer =
        (desc.validationFlags & (ValidationFlags::Api | ValidationFlags::GpuAssisted)) != ValidationFlags::None;
    if (wantDebugLayer)
    {
        ID3D12Debug1* debug = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            if ((desc.validationFlags & ValidationFlags::GpuAssisted) != ValidationFlags::None)
            {
                debug->SetEnableGPUBasedValidation(TRUE);
            }
            instance->debugLayer = true;
            release(debug);
        }
        else
        {
            logMessage(sink, MessageSeverity::Warning, MessageSource::Slag,
                       "slag: createInstance: the D3D12 debug layer is not available; D3D12 validation is off");
        }
    }
    if ((desc.validationFlags & ValidationFlags::Synchronization) != ValidationFlags::None)
    {
        logMessage(sink, MessageSeverity::Warning, MessageSource::Slag,
                   "slag: createInstance: D3D12 has no synchronization validation; GPU-based validation covers "
                   "some of it");
    }

    // The DXGI debug flag needs the Graphics Tools feature; fall back without it.
    HRESULT result = E_FAIL;
    if (instance->debugLayer)
    {
        result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&instance->factory));
    }
    if (FAILED(result))
    {
        result = CreateDXGIFactory2(0, IID_PPV_ARGS(&instance->factory));
    }
    if (FAILED(result))
    {
        logMessage(sink, MessageSeverity::Error, MessageSource::Slag,
                   "slag: createInstance: CreateDXGIFactory2 failed (0x%08lx)", static_cast<unsigned long>(result));
        destroyImpl(instance);
        return Status::Unsupported;
    }

    Status status = listDevices(*instance, desc);
    if (status != Status::Ok)
    {
        destroyImpl(instance);
        return status;
    }
    return static_cast<InstanceCommon*>(instance);
}

void destroyInstance(InstanceCommon* instance)
{
    destroyImpl(static_cast<InstanceImpl*>(instance));
}

} // namespace slag::d3d12
