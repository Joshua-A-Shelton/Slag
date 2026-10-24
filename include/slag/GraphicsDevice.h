#pragma once

// GraphicsDevice: info, capabilities, initialization, create/destroy, descriptor
// writes, placement queries, semaphore waits, status, memory budget, and the
// pipeline cache (ADRs 0008, 0015, 0019, 0023, 0029, 0031, 0043, 0053, 0054).

#include <cstdint>

namespace slag {

enum class GraphicsDeviceType : uint8_t
{
    Discrete,
    Integrated,
    Software,
};

struct GraphicsDeviceInfo
{
    char name[256] = {};
    char driverVersion[64] = {}; // for logging only; vendors encode versions differently
    uint32_t vendorId = 0;
    uint32_t deviceId = 0;
    GraphicsDeviceType type = GraphicsDeviceType::Discrete;
    uint64_t dedicatedMemoryBytes = 0;
    uint64_t sharedMemoryBytes = 0;
    uint8_t luid[8] = {}; // all zero where the platform has no LUID (Linux)
};

class GraphicsDevice
{
public:
    const GraphicsDeviceInfo& info() const;

    uint64_t userData() const;
    void setUserData(uint64_t value);

    GraphicsDevice(const GraphicsDevice&) = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

protected:
    GraphicsDevice() = default;
    ~GraphicsDevice() = default;
};

} // namespace slag
