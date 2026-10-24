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
};

inline void collectTestMessage(slag::MessageSeverity severity, slag::MessageSource, const char* message, void* userData)
{
    if (severity == slag::MessageSeverity::Error)
    {
        static_cast<TestMessages*>(userData)->errors.push_back(message);
    }
}

class TestInstance
{
public:
    explicit TestInstance(slag::Backend backend,
                          slag::ValidationFailureAction action = slag::ValidationFailureAction::Continue)
    {
        slag::InstanceDesc desc;
        desc.backend = backend;
        desc.validation = slag::ValidationFlags::Api;
        desc.validationFailureAction = action;
        desc.debugCallback = collectTestMessage;
        desc.debugUserData = &messages;
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
