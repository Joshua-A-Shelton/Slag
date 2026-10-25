#pragma once

// Result<T> and Status (ADR 0060).

#include <cstdint>

namespace slag {

enum class Status : int32_t
{
    // Success: the value is valid.
    Ok = 0,
    Suboptimal,
    Occluded,
    Timeout,

    // Errors: no value.
    OutOfDate = -1,
    DeviceLost = -2,
    OutOfDeviceMemory = -3,
    OutOfHostMemory = -4,
    Unsupported = -5,
};

// [[nodiscard]]: ignoring a result warns; write (void) to ignore one on purpose.
template <class T>
class [[nodiscard]] Result
{
public:
    Result(T value)
        : code(Status::Ok)
        , payload(value)
    {}

    Result(Status status, T value)
        : code(status)
        , payload(value)
    {}

    Result(Status status)
        : code(status)
        , payload()
    {}

    // True for every success status, including Suboptimal and Timeout.
    explicit operator bool() const
    {
        return static_cast<int32_t>(code) >= 0;
    }

    Status status() const
    {
        return code;
    }

    // Only valid when the result converts to true.
    T& value()
    {
        return payload;
    }

    const T& value() const
    {
        return payload;
    }

private:
    Status code;
    T payload;
};

template <>
class [[nodiscard]] Result<void>
{
public:
    Result()
        : code(Status::Ok)
    {}

    Result(Status status)
        : code(status)
    {}

    explicit operator bool() const
    {
        return static_cast<int32_t>(code) >= 0;
    }

    Status status() const
    {
        return code;
    }

private:
    Status code;
};

} // namespace slag
