#include "TestDevice.h"

#include <slag/Buffer.h>
#include <slag/GraphicsDevice.h>

#include <cstring>

namespace {

void testBufferKinds(slag::Backend backend)
{
    TestInstance test(backend);
    slag::GraphicsDevice* device = test.firstDevice();
    REQUIRE(device);
    REQUIRE(static_cast<bool>(device->initialize({})));

    SUBCASE("GpuOnly has an address but no CPU pointer")
    {
        slag::Result<slag::Buffer*> buffer = device->createBuffer(
            {.size = 1 << 20, .placement = slag::Placement::automatic(slag::MemoryKind::GpuOnly), .debugName = "gpu"});
        REQUIRE(static_cast<bool>(buffer));
        CHECK(buffer.value()->gpuAddress() != 0);
        CHECK(buffer.value()->mappedData() == nullptr);
        device->destroy(buffer.value());
    }

    SUBCASE("Upload and Readback are mapped for their lifetime")
    {
        for (slag::MemoryKind kind : {slag::MemoryKind::Upload, slag::MemoryKind::Readback})
        {
            slag::Result<slag::Buffer*> buffer =
                device->createBuffer({.size = 4096, .placement = slag::Placement::automatic(kind), .userData = 7});
            REQUIRE(static_cast<bool>(buffer));
            REQUIRE(buffer.value()->mappedData());
            CHECK(buffer.value()->userData() == 7);
            std::memset(buffer.value()->mappedData(), 0xAB, 4096);
            CHECK(static_cast<unsigned char*>(buffer.value()->mappedData())[4095] == 0xAB);
            device->destroy(buffer.value());
        }
    }

    SUBCASE("GpuUpload works exactly when the capability says so")
    {
        slag::Result<slag::Buffer*> buffer =
            device->createBuffer({.size = 4096, .placement = slag::Placement::automatic(slag::MemoryKind::GpuUpload)});
        CHECK(static_cast<bool>(buffer) == device->capabilities().gpuUploadMemory);
        if (buffer)
        {
            CHECK(buffer.value()->mappedData() != nullptr);
            device->destroy(buffer.value());
        }
        else
        {
            CHECK(buffer.status() == slag::Status::Unsupported);
            test.messages.errors.clear(); // the expected "no suitable memory" message
        }
        MESSAGE("GpuUpload " << std::string(buffer ? "supported" : "not supported") << " on "
                             << std::string(device->info().name));
    }

    device->shutdown();
    test.checkNoErrors();
}

} // namespace

#ifdef SLAG_BACKEND_VULKAN
TEST_CASE("Vulkan: buffers in each memory kind")
{
    testBufferKinds(slag::Backend::Vulkan);
}
#endif

#ifdef SLAG_BACKEND_D3D12
TEST_CASE("D3D12: buffers in each memory kind")
{
    testBufferKinds(slag::Backend::D3D12);
}
#endif
