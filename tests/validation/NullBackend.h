#pragma once

// The null test backend (NullBackend.cpp).

#include "common/Backend.h"
#include "common/Objects.h"

namespace slag {

// External backends use 0x80 and up (ADR 0065).
constexpr Backend nullBackend = static_cast<Backend>(0xF0);

const BackendInfo* slagBackend_null();

} // namespace slag

namespace slag::null {

#define SLAG_BACKEND_FN(ReturnType, name, parameters) ReturnType name parameters;
#include "common/BackendFunctions.inl"
#undef SLAG_BACKEND_FN

struct GraphicsDeviceImpl : GraphicsDeviceCommon
{};

struct NullSemaphore : SemaphoreCommon
{
    uint64_t value = 0;
};

} // namespace slag::null
