#include "vulkan/VkBackend.h"

namespace slag::vk {

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

    VkSemaphoreTypeCreateInfo typeInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
    typeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    typeInfo.initialValue = desc.initialValue;
    VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    createInfo.pNext = &typeInfo;
    VkResult result = device.fn.vkCreateSemaphore(device.device, &createInfo, nullptr, &semaphore->semaphore);
    if (result != VK_SUCCESS)
    {
        device.instance->allocator.destroy(semaphore);
        return toStatus(device, result);
    }
    setObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(semaphore->semaphore), desc.debugName);
    return static_cast<SemaphoreCommon*>(semaphore);
}

void destroySemaphore(GraphicsDeviceCommon* common, SemaphoreCommon* object)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    auto* semaphore = static_cast<SemaphoreImpl*>(object);
    device.fn.vkDestroySemaphore(device.device, semaphore->semaphore, nullptr);
    destroyObject(device.instance, semaphore);
}

uint64_t semaphoreValue(const SemaphoreCommon* object)
{
    auto* semaphore = static_cast<const SemaphoreImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(semaphore->device);
    uint64_t value = 0;
    VkResult result = device.fn.vkGetSemaphoreCounterValue(device.device, semaphore->semaphore, &value);
    if (result == VK_ERROR_DEVICE_LOST)
    {
        // D3D12 fences read UINT64_MAX once the device is lost; match that (ADR 0054).
        reportDeviceLost(&device);
        return UINT64_MAX;
    }
    return value;
}

Result<void> signalSemaphore(SemaphoreCommon* object, uint64_t value)
{
    auto* semaphore = static_cast<SemaphoreImpl*>(object);
    auto& device = *static_cast<GraphicsDeviceImpl*>(semaphore->device);
    VkSemaphoreSignalInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO};
    info.semaphore = semaphore->semaphore;
    info.value = value;
    return toStatus(device, device.fn.vkSignalSemaphore(device.device, &info));
}

Result<void> waitSemaphores(GraphicsDeviceCommon* common, std::span<const SemaphoreValue> values, WaitMode mode,
                            uint64_t timeoutNs)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    Vector<VkSemaphore> semaphores(&device.instance->allocator);
    Vector<uint64_t> waitValues(&device.instance->allocator);
    if (!semaphores.reserve(values.size()) || !waitValues.reserve(values.size()))
    {
        return Status::OutOfHostMemory;
    }
    for (const SemaphoreValue& value : values)
    {
        (void)semaphores.pushBack(static_cast<SemaphoreImpl*>(value.semaphore)->semaphore);
        (void)waitValues.pushBack(value.value);
    }
    VkSemaphoreWaitInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
    info.flags = mode == WaitMode::Any ? VK_SEMAPHORE_WAIT_ANY_BIT : 0;
    info.semaphoreCount = static_cast<uint32_t>(semaphores.size());
    info.pSemaphores = semaphores.data();
    info.pValues = waitValues.data();
    return toStatus(device, device.fn.vkWaitSemaphores(device.device, &info, timeoutNs));
}

} // namespace slag::vk
