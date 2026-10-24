#include "vulkan/VkBackend.h"

#include <cstdio>

// Crash diagnostics through VK_EXT_device_fault (ADR 0014): the fault's
// description, the faulting addresses, and vendor-specific details.

namespace slag::vk {

namespace {

const char* addressTypeName(VkDeviceFaultAddressTypeEXT type)
{
    switch (type)
    {
    case VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT:
        return "invalid read";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_WRITE_INVALID_EXT:
        return "invalid write";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_EXECUTE_INVALID_EXT:
        return "invalid execute";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_UNKNOWN_EXT:
        return "instruction pointer (unknown)";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_INVALID_EXT:
        return "instruction pointer (invalid)";
    case VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT:
        return "instruction pointer (fault)";
    default:
        return "no address";
    }
}

} // namespace

bool describeDeviceLoss(GraphicsDeviceCommon* common, char* text, size_t size)
{
    auto& device = *static_cast<GraphicsDeviceImpl*>(common);
    if (!device.device || !device.deviceFault || !device.fn.vkGetDeviceFaultInfoEXT)
    {
        return false;
    }
    constexpr uint32_t maxEntries = 16;
    VkDeviceFaultCountsEXT counts = {VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT};
    VkResult result = device.fn.vkGetDeviceFaultInfoEXT(device.device, &counts, nullptr);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        return false;
    }
    VkDeviceFaultAddressInfoEXT addresses[maxEntries] = {};
    VkDeviceFaultVendorInfoEXT vendors[maxEntries] = {};
    counts.addressInfoCount = counts.addressInfoCount < maxEntries ? counts.addressInfoCount : maxEntries;
    counts.vendorInfoCount = counts.vendorInfoCount < maxEntries ? counts.vendorInfoCount : maxEntries;
    counts.vendorBinarySize = 0; // vendor tools only
    VkDeviceFaultInfoEXT info = {VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT};
    info.pAddressInfos = addresses;
    info.pVendorInfos = vendors;
    result = device.fn.vkGetDeviceFaultInfoEXT(device.device, &counts, &info);
    if (result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        return false;
    }

    size_t used = 0;
    auto add = [&](const char* format, auto... values) {
        if (used + 1 < size)
        {
            int written = std::snprintf(text + used, size - used, format, values...);
            used += written > 0
                        ? (static_cast<size_t>(written) < size - used ? static_cast<size_t>(written) : size - used - 1)
                        : 0;
        }
    };
    text[0] = '\0';
    add("slag: crash report for \"%s\" (VK_EXT_device_fault): %s", device.info.name,
        info.description[0] ? info.description : "no description");
    for (uint32_t i = 0; i < counts.addressInfoCount; ++i)
    {
        // The fault lies within reportedAddress rounded to addressPrecision.
        add("\n  %s at GPU address 0x%llx (precision %llu bytes)", addressTypeName(addresses[i].addressType),
            static_cast<unsigned long long>(addresses[i].reportedAddress),
            static_cast<unsigned long long>(addresses[i].addressPrecision));
    }
    for (uint32_t i = 0; i < counts.vendorInfoCount; ++i)
    {
        add("\n  vendor: %s (code 0x%llx, data 0x%llx)", vendors[i].description,
            static_cast<unsigned long long>(vendors[i].vendorFaultCode),
            static_cast<unsigned long long>(vendors[i].vendorFaultData));
    }
    if (counts.addressInfoCount == 0 && counts.vendorInfoCount == 0)
    {
        add("\n  the driver reported no addresses or vendor details");
    }
    return true;
}

} // namespace slag::vk
