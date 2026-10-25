#pragma once

// Shared setup for the validation tests: an instance on the null backend,
// its device, and a check that a statement reports one validation error.

#include "NullBackend.h"

#include <slag/Slag.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

struct ValidationMessages
{
    std::vector<std::string> errors;
};

inline void collectValidation(const slag::DebugMessage& debugMessage, void* userData)
{
    slag::MessageSeverity severity = debugMessage.severity;
    const char* message = debugMessage.text;
    if (severity == slag::MessageSeverity::Error)
    {
        static_cast<ValidationMessages*>(userData)->errors.push_back(message);
    }
}

// The messages after `from`, one per line, for failure output.
inline std::string joinErrors(const std::vector<std::string>& errors, size_t from)
{
    std::string text;
    for (size_t i = from; i < errors.size(); ++i)
    {
        text += "\n  " + errors[i];
    }
    return text;
}

class ValidationFixture
{
public:
    explicit ValidationFixture(bool initialize = true, const slag::GraphicsDeviceInitDesc& initDesc = {})
    {
        slag::InstanceDesc desc;
        desc.backend = slag::nullBackend;
        desc.validationFailureAction = slag::ValidationFailureAction::Continue;
        desc.messageCallback = collectValidation;
        desc.messageUserData = &messages;
        instance = slag::createInstance(desc).value();
        REQUIRE(instance);
        device = instance->graphicsDevices()[0];
        common = static_cast<slag::GraphicsDeviceCommon*>(device);
        if (initialize)
        {
            REQUIRE(static_cast<bool>(device->initialize(initDesc)));
        }
    }

    ~ValidationFixture()
    {
        if (common->initialized)
        {
            device->shutdown();
        }
        slag::destroy(instance);
    }

    // Resources most tests need.
    slag::Buffer* buffer(uint64_t size = 1024, slag::MemoryKind kind = slag::MemoryKind::GpuOnly)
    {
        return device->createBuffer({.size = size, .placement = slag::Placement::automatic(kind)}).value();
    }
    slag::Texture* texture(slag::TextureUsage usage = slag::TextureUsage::None, uint32_t size = 64,
                           slag::PixelFormat format = slag::PixelFormat::R8G8B8A8Unorm, uint32_t samples = 1,
                           uint32_t mips = 1)
    {
        return device
            ->createTexture2D({.format = format,
                               .width = size,
                               .height = size,
                               .mipCount = mips,
                               .sampleCount = samples,
                               .usage = usage})
            .value();
    }
    slag::CommandBuffer* recording(slag::CommandQueueType type = slag::CommandQueueType::Graphics)
    {
        slag::CommandBuffer* commandBuffer = device->createCommandBuffer({.queueType = type}).value();
        REQUIRE(commandBuffer);
        REQUIRE(static_cast<bool>(commandBuffer->begin()));
        return commandBuffer;
    }
    slag::Pipeline* computePipeline()
    {
        return device->createComputePipeline({.compute = {.binaries = binaries}}).value();
    }
    slag::Pipeline* graphicsPipeline(slag::PixelFormat color = slag::PixelFormat::R8G8B8A8Unorm,
                                     slag::PixelFormat depth = slag::PixelFormat::Undefined, uint32_t samples = 1)
    {
        colorFormats[0] = color;
        return device
            ->createGraphicsPipeline({.vertex = {.binaries = binaries},
                                      .pixel = {.binaries = binaries},
                                      .colorFormats = {colorFormats, color == slag::PixelFormat::Undefined ? 0u : 1u},
                                      .depthStencilFormat = depth,
                                      .sampleCount = samples})
            .value();
    }

    // Shaders are never looked at by the null backend; a fake SPIR-V blob.
    const uint32_t spirvWords[5] = {0x07230203, 0x00010600, 0, 1, 0};
    slag::ShaderBinary binaries[1] = {{slag::ShaderFormat::Spirv, spirvWords, sizeof(spirvWords)}};
    slag::PixelFormat colorFormats[1] = {};

    ValidationMessages messages;
    slag::Instance* instance = nullptr;
    slag::GraphicsDevice* device = nullptr;
    slag::GraphicsDeviceCommon* common = nullptr;
};

// `statement` must report exactly one validation error containing `text`.
#define CHECK_VALIDATION(fixture, text, statement)                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        size_t before_ = (fixture).messages.errors.size();                                                             \
        statement;                                                                                                     \
        std::vector<std::string>& errors_ = (fixture).messages.errors;                                                 \
        CHECK_MESSAGE(errors_.size() == before_ + 1,                                                                   \
                      "expected one error with \"" << (text) << "\", got:" << joinErrors(errors_, before_));           \
        if (errors_.size() > before_)                                                                                  \
        {                                                                                                              \
            CHECK_MESSAGE(errors_[before_].find(text) != std::string::npos, errors_[before_]);                         \
        }                                                                                                              \
        errors_.resize(before_);                                                                                       \
    } while (false)

// `statement` must report `count` validation errors, the first containing `text`.
#define CHECK_VALIDATIONS(fixture, count, text, statement)                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        size_t before_ = (fixture).messages.errors.size();                                                             \
        statement;                                                                                                     \
        std::vector<std::string>& errors_ = (fixture).messages.errors;                                                 \
        CHECK(errors_.size() == before_ + (count));                                                                    \
        if (errors_.size() > before_)                                                                                  \
        {                                                                                                              \
            CHECK_MESSAGE(errors_[before_].find(text) != std::string::npos, errors_[before_]);                         \
        }                                                                                                              \
        errors_.resize(before_);                                                                                       \
    } while (false)

// `statement` must report no validation error.
#define CHECK_NO_VALIDATION(fixture, statement)                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        size_t before_ = (fixture).messages.errors.size();                                                             \
        statement;                                                                                                     \
        CHECK_MESSAGE((fixture).messages.errors.size() == before_,                                                     \
                      ((fixture).messages.errors.size() > before_ ? (fixture).messages.errors.back() : ""));           \
        (fixture).messages.errors.resize(before_);                                                                     \
    } while (false)
