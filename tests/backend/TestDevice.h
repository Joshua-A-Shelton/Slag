#pragma once

// Shared setup for backend tests: one instance per backend, the first listed
// device, and a debug callback that collects errors.

#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>

#include <doctest/doctest.h>

#include <string>
#include <vector>

struct TestMessages
{
    std::vector<std::string> errors;
    std::vector<std::string> slagInfo; // Slag's own Info messages
};

inline void collectTestMessage(const slag::DebugMessage& debugMessage, void* userData)
{
    slag::MessageSeverity severity = debugMessage.severity;
    slag::MessageSource source = debugMessage.source;
    const char* message = debugMessage.text;
    if (severity == slag::MessageSeverity::Error)
    {
        static_cast<TestMessages*>(userData)->errors.push_back(message);
    }
    else if (severity == slag::MessageSeverity::Info && source == slag::MessageSource::Slag)
    {
        static_cast<TestMessages*>(userData)->slagInfo.push_back(message);
    }
}

class TestInstance
{
public:
    explicit TestInstance(slag::Backend backend,
                          slag::ValidationFailureAction action = slag::ValidationFailureAction::Continue,
                          slag::ValidationFlags validationFlags = slag::ValidationFlags::Api)
    {
        slag::InstanceDesc desc;
        desc.backend = backend;
        desc.validationFlags = validationFlags;
        desc.validationFailureAction = action;
        // Software devices (WARP) are tested too, and are all hosted CI has.
        desc.allowSoftwareDevices = true;
        desc.messageCallback = collectTestMessage;
        desc.messageUserData = &messages;
        desc.applicationName = "slag_backend_tests";
        slag::Result<slag::Instance*> result = slag::createInstance(desc);
        if (result)
        {
            instance = result.value();
        }
    }

    ~TestInstance()
    {
        slag::destroy(instance);
    }

    slag::GraphicsDevice* firstDevice() const
    {
        return instance && !instance->graphicsDevices().empty() ? instance->graphicsDevices()[0] : nullptr;
    }

    void checkNoErrors() const
    {
        for (const std::string& error : messages.errors)
        {
            MESSAGE("error: " << error);
        }
        CHECK(messages.errors.empty());
    }

    slag::Instance* instance = nullptr;
    TestMessages messages;
};
