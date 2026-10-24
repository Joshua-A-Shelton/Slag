#include "vulkan/VkBackend.h"

namespace slag {

const BackendInfo* slagBackend_vulkan()
{
    static const BackendFunctions functions = {
#define SLAG_BACKEND_FN(ReturnType, name, parameters) &vk::name,
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN
    };
    static const BackendInfo info = {Backend::Vulkan, "Vulkan", &functions};
    return &info;
}

} // namespace slag
