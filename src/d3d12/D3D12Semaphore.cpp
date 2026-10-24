#include "d3d12/D3D12Backend.h"

namespace slag::d3d12 {

Result<SemaphoreCommon*> createSemaphore(GraphicsDeviceCommon* common, const SemaphoreDesc& desc)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* semaphore = device.instance->allocator.create<SemaphoreImpl>();
    if (!semaphore)
    {
        return Status::OutOfHostMemory;
    }
    semaphore->device = &device;
    semaphore->functions = device.functions;
    HRESULT result =
        device.device->CreateFence(desc.initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&semaphore->fence));
    if (FAILED(result))
    {
        device.instance->allocator.destroy(semaphore);
        return toStatus(device, result);
    }
    setObjectName(semaphore->fence, desc.debugName);
    return static_cast<SemaphoreCommon*>(semaphore);
}

void destroySemaphore(GraphicsDeviceCommon* common, SemaphoreCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* semaphore = static_cast<SemaphoreImpl*>(object);
    release(semaphore->fence);
    device.instance->allocator.destroy(semaphore);
}

uint64_t semaphoreValue(const SemaphoreCommon* object)
{
    auto* semaphore = static_cast<const SemaphoreImpl*>(object);
    uint64_t value = semaphore->fence->GetCompletedValue();
    if (value == UINT64_MAX)
    {
        // Fences read UINT64_MAX once the device is lost (ADR 0054).
        auto& device = *static_cast<GraphicsDeviceImpl*>(semaphore->device);
        (void)toStatus(device, device.device->GetDeviceRemovedReason());
    }
    return value;
}

Result<void> signalSemaphore(SemaphoreCommon* object, uint64_t value)
{
    auto* semaphore = static_cast<SemaphoreImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(semaphore->device);
    return toStatus(device, semaphore->fence->Signal(value));
}

Result<void> waitSemaphores(GraphicsDeviceCommon* common, std::span<const SemaphoreValue> values, WaitMode mode,
                            uint64_t timeoutNs)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    Vector<ID3D12Fence*> fences(&device.instance->allocator);
    Vector<UINT64> waitValues(&device.instance->allocator);
    if (!fences.reserve(values.size()) || !waitValues.reserve(values.size()))
    {
        return Status::OutOfHostMemory;
    }
    for (const SemaphoreValue& value : values)
    {
        (void)fences.pushBack(static_cast<SemaphoreImpl*>(value.semaphore)->fence);
        (void)waitValues.pushBack(value.value);
    }

    // An event per call keeps concurrent waits independent (ADR 0068).
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!event)
    {
        return Status::OutOfHostMemory;
    }
    D3D12_MULTIPLE_FENCE_WAIT_FLAGS flags =
        mode == WaitMode::Any ? D3D12_MULTIPLE_FENCE_WAIT_FLAG_ANY : D3D12_MULTIPLE_FENCE_WAIT_FLAG_ALL;
    HRESULT result = device.device->SetEventOnMultipleFenceCompletion(fences.data(), waitValues.data(),
                                                                      static_cast<UINT>(fences.size()), flags, event);
    Status status = toStatus(device, result);
    if (status == Status::Ok)
    {
        // Milliseconds, rounded up so a short timeout still waits; INFINITE is reserved.
        uint64_t milliseconds = timeoutNs / 1'000'000 + (timeoutNs % 1'000'000 != 0 ? 1 : 0);
        DWORD timeout = timeoutNs == UINT64_MAX    ? INFINITE
                        : milliseconds >= INFINITE ? INFINITE - 1
                                                   : static_cast<DWORD>(milliseconds);
        DWORD waited = WaitForSingleObject(event, timeout);
        status = waited == WAIT_OBJECT_0 ? Status::Ok : waited == WAIT_TIMEOUT ? Status::Timeout : Status::Unsupported;
    }
    CloseHandle(event);
    if (status == Status::Ok)
    {
        status = toStatus(device, device.device->GetDeviceRemovedReason());
    }
    return status;
}

} // namespace slag::d3d12
