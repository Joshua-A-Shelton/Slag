#pragma once

// Instance, InstanceDesc, backend-options union, debug message callback (ADRs 0008, 0010, 0011, 0065, 0066, 0067).

#include <slag/Flags.h>
#include <slag/Result.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace slag {

class GraphicsDevice;

// Values from 0x80 up are reserved for external backends (ADR 0065).
enum class Backend : uint8_t
{
    D3D12 = 0,
    Vulkan = 1,
};

// Backend validation layers (ADR 0010). Slag's own checks are controlled by
// SLAG_VALIDATION at compile time (ADR 0067).
enum class ValidationFlags : uint32_t
{
    None = 0,
    Api = 1u << 0,
    GpuAssisted = 1u << 1,
    Synchronization = 1u << 2,
};
SLAG_FLAGS(ValidationFlags)

enum class ValidationFailureAction : uint8_t
{
    Break,
    Continue,
    Abort,
};

enum class MessageSeverity : uint8_t
{
    Error,
    Warning,
    Info,
};

enum class MessageSource : uint8_t
{
    Slag,
    Backend,
};

enum class DevicePreference : uint8_t
{
    HighPerformance,
    LowPower,
};

// May be called from any thread, concurrently. `message` is valid only during
// the call.
using DebugMessageFn = void (*)(MessageSeverity severity, MessageSource source, const char* message, void* userData);

// Slag's own CPU allocations (ADR 0066). Must be thread-safe. Null callbacks
// mean aligned malloc/free.
struct AllocatorCallbacks
{
    void* (*allocate)(size_t size, size_t alignment, void* userData) = nullptr;
    void (*free)(void* pointer, size_t size, void* userData) = nullptr;
    void* userData = nullptr;
};

// Backend-specific instance options (ADR 0010). Empty in v1.
struct D3D12Options
{};

struct VulkanOptions
{};

union BackendOptions
{
    D3D12Options d3d12 = {};
    VulkanOptions vulkan;
#ifdef SLAG_EXTERNAL_BACKEND_OPTIONS
#include SLAG_EXTERNAL_BACKEND_OPTIONS
#endif
};

struct InstanceDesc
{
#ifdef _WIN32
    Backend backend = Backend::D3D12;
#else
    Backend backend = Backend::Vulkan;
#endif
    ValidationFlags validation = ValidationFlags::None;
    ValidationFailureAction validationFailureAction = ValidationFailureAction::Break;
    DebugMessageFn debugCallback = nullptr;
    void* debugUserData = nullptr;
    AllocatorCallbacks allocator;
    DevicePreference devicePreference = DevicePreference::HighPerformance;
    bool allowSoftwareDevices = false;
    bool crashDiagnostics = false;
    const char* applicationName = nullptr;
    uint32_t applicationVersion = 0;
    const char* engineName = nullptr;
    uint32_t engineVersion = 0;
    BackendOptions backendOptions;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class Instance
{
public:
    // Devices meeting the baseline, in preference order (ADRs 0008, 0012).
    std::span<GraphicsDevice* const> graphicsDevices() const;

    uint64_t userData() const;
    void setUserData(uint64_t value);

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;

protected:
    Instance() = default;
    ~Instance() = default;
};

Result<Instance*> createInstance(const InstanceDesc& desc);
void destroy(Instance* instance);
bool isBackendAvailable(Backend backend);

} // namespace slag
