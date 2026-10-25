#include "common/Logging.h"
#include "common/Objects.h"

#include <slag/Instance.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace {

struct CapturedMessage
{
    slag::MessageSeverity severity;
    slag::MessageSource source;
    std::string text;
};

void captureMessage(const slag::DebugMessage& debugMessage, void* userData)
{
    slag::MessageSeverity severity = debugMessage.severity;
    slag::MessageSource source = debugMessage.source;
    const char* message = debugMessage.text;
    static_cast<std::vector<CapturedMessage>*>(userData)->push_back({severity, source, message});
}

} // namespace

TEST_CASE("isBackendAvailable matches the compiled backends")
{
#ifdef SLAG_BACKEND_VULKAN
    CHECK(slag::isBackendAvailable(slag::Backend::Vulkan));
#else
    CHECK_FALSE(slag::isBackendAvailable(slag::Backend::Vulkan));
#endif
#ifdef SLAG_BACKEND_D3D12
    CHECK(slag::isBackendAvailable(slag::Backend::D3D12));
#else
    CHECK_FALSE(slag::isBackendAvailable(slag::Backend::D3D12));
#endif
}

#ifndef SLAG_BACKEND_D3D12
TEST_CASE("createInstance reports a backend that isn't compiled in")
{
    std::vector<CapturedMessage> messages;
    slag::InstanceDesc desc;
    desc.backend = slag::Backend::D3D12;
    desc.messageCallback = captureMessage;
    desc.messageUserData = &messages;

    slag::Result<slag::Instance*> instance = slag::createInstance(desc);
    CHECK(instance.status() == slag::Status::Unsupported);
    REQUIRE(messages.size() == 1);
    CHECK(messages[0].severity == slag::MessageSeverity::Error);
    CHECK(messages[0].source == slag::MessageSource::Slag);
}
#endif

TEST_CASE("validationFailure reports through the debug callback and continues")
{
    std::vector<CapturedMessage> messages;
    slag::InstanceCommon instance;
    instance.messageCallback = captureMessage;
    instance.messageUserData = &messages;
    instance.failureAction = slag::ValidationFailureAction::Continue;

    slag::validationFailure(&instance, "CommandBuffer::draw", "no render pass is active (%s)", "MainPass");

    REQUIRE(messages.size() == 1);
    CHECK(messages[0].severity == slag::MessageSeverity::Error);
    CHECK(messages[0].source == slag::MessageSource::Slag);
    CHECK(messages[0].text == "slag: CommandBuffer::draw: no render pass is active (MainPass)");
}
