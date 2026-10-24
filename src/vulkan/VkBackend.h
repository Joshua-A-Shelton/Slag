#pragma once

// Vulkan backend entry points, one per line in BackendFunctions.inl.

#include "common/Backend.h"
#include "common/Objects.h"

namespace slag::vk {

#define SLAG_BACKEND_FN(ReturnType, name, parameters) ReturnType name parameters;
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN

} // namespace slag::vk
