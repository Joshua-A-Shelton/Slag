#include <slag/GraphicsDevice.h>
#include <slag/Instance.h>

#include <doctest/doctest.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Messages
{
    std::vector<std::string> errors;
    std::vector<std::string> other;
};

void collect(slag::MessageSeverity severity, slag::MessageSource, const char* message, void* userData)
{
    auto* messages = static_cast<Messages*>(userData);
    (severity == slag::MessageSeverity::Error ? messages->errors : messages->other).push_back(message);
}

const char* typeName(slag::GraphicsDeviceType type)
{
    switch (type)
    {
    case slag::GraphicsDeviceType::Discrete:
        return "discrete";
    case slag::GraphicsDeviceType::Integrated:
        return "integrated";
    case slag::GraphicsDeviceType::Software:
        return "software";
    }
    return "?";
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: createInstance lists devices meeting the baseline")
{
    Messages messages;
    slag::InstanceDesc desc;
    desc.backend = slag::Backend::Vulkan;
    desc.validationFlags = slag::ValidationFlags::Api;
    desc.messageCallback = collect;
    desc.messageUserData = &messages;
    desc.applicationName = "slag_backend_tests";

    slag::Result<slag::Instance*> result = slag::createInstance(desc);
    REQUIRE(static_cast<bool>(result));
    slag::Instance* instance = result.value();

    std::span<slag::GraphicsDevice* const> devices = instance->graphicsDevices();
    CHECK_FALSE(devices.empty());
    for (slag::GraphicsDevice* device : devices)
    {
        const slag::GraphicsDeviceInfo& info = device->info();
        MESSAGE(info.name << " (" << std::string(typeName(info.deviceType)) << ", " << info.driverVersion << ", "
                          << info.dedicatedMemoryBytes / (1024 * 1024) << " MiB dedicated)");
        CHECK(std::strlen(info.name) > 0);
        CHECK(info.vendorId != 0);
    }

    // Preference order: no device ranks better than the one before it.
    for (size_t i = 1; i < devices.size(); ++i)
    {
        bool previousDiscrete = devices[i - 1]->info().deviceType == slag::GraphicsDeviceType::Discrete;
        bool currentDiscrete = devices[i]->info().deviceType == slag::GraphicsDeviceType::Discrete;
        CHECK((previousDiscrete || !currentDiscrete));
    }

    devices[0]->setUserData(42);
    CHECK(devices[0]->userData() == 42);

    slag::destroy(instance);
    for (const std::string& error : messages.errors)
    {
        MESSAGE("error: " << error);
    }
    CHECK(messages.errors.empty());
}
#endif
