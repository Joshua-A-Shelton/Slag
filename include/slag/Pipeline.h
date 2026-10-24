#pragma once

// Pipeline, pipeline Descs, state structs, ShaderStage, VertexInputLayout (ADRs 0036–0043).

#include <cstddef>
#include <cstdint>
#include <span>

namespace slag {

// Values from 0x80 up are reserved for external backends (ADR 0036).
enum class ShaderFormat : uint8_t
{
    Dxil,
    Spirv,
};

struct ShaderBinary
{
    ShaderFormat format = ShaderFormat::Spirv;
    const void* data = nullptr;
    size_t size = 0;
};

// The first binary the backend accepts is used (ADR 0036). Only needs to live
// until the create call returns.
struct ShaderStage
{
    std::span<const ShaderBinary> binaries;
    const char* entryPoint = "main";
};

struct ComputePipelineDesc
{
    ShaderStage compute;
    const char* debugName = nullptr;
    uint64_t userData = 0;
};

class Pipeline
{
public:
    uint64_t userData() const;
    void setUserData(uint64_t value);

    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;

protected:
    Pipeline() = default;
    ~Pipeline() = default;
};

} // namespace slag
