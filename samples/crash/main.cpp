// Loses the device on purpose to show crash diagnostics (ADR 0014): a compute
// shader writes through the descriptor of a buffer whose memory block was
// destroyed, which page-faults. The block is large and the writes hit only its
// last quarter: drivers reuse freed address ranges for their own small
// allocations (a small block came back as DRED's breadcrumb memory, which
// the writes then corrupted), and they tend to start at the low end. With InstanceDesc::crashDiagnostics, Slag
// sends a crash report through the debug callback when it sees the loss.
//
// Not a CTest test: it resets the GPU. Run it by hand:
//   crash --backend d3d12
//   crash --backend vulkan
// Exits 0 when the device was lost and a crash report arrived.

#include "SampleApp.h"

#include <cstdio>
#include <cstring>

namespace {

struct Messages
{
    bool crashReport = false;
};

void printMessage(slag::MessageSeverity severity, slag::MessageSource, const char* message, void* userData)
{
    if (severity == slag::MessageSeverity::Info)
    {
        return; // the Vulkan loader is chatty
    }
    std::printf("[%s] %s\n", severity == slag::MessageSeverity::Error ? "error" : "warning", message);
    if (std::strstr(message, "crash report"))
    {
        static_cast<Messages*>(userData)->crashReport = true;
    }
}

const char* statusName(slag::Status status)
{
    switch (status)
    {
    case slag::Status::Ok:
        return "Ok";
    case slag::Status::Timeout:
        return "Timeout";
    case slag::Status::DeviceLost:
        return "DeviceLost";
    default:
        return "another status";
    }
}

} // namespace

int main(int argc, char** argv)
{
    SampleOptions options = parseOptions(argc, argv);
    Messages messages;
    slag::InstanceDesc desc;
    desc.backend = options.backend;
    desc.crashDiagnostics = true;
    desc.messageCallback = printMessage;
    desc.messageUserData = &messages;
    desc.applicationName = "slag crash sample";
    slag::Result<slag::Instance*> instance = slag::createInstance(desc);
    if (!instance || instance.value()->graphicsDevices().empty())
    {
        std::fprintf(stderr, "no device\n");
        return 1;
    }
    slag::GraphicsDevice* device = instance.value()->graphicsDevices()[0];
    std::printf("device: %s\n", device->info().name);
    if (!device->initialize({}))
    {
        return 1;
    }

    SampleShader shader("Crash", "computeMain");
    slag::Pipeline* pipeline =
        device->createComputePipeline({.compute = shader.stage("computeMain"), .debugName = "crash"}).value();
    // Writes hit only the last quarter of a large block: drivers reuse freed
    // address ranges for their own small allocations, from the low end (a
    // small block came back as DRED's breadcrumb memory, and the writes
    // corrupted it instead of faulting).
    constexpr uint64_t blockSize = 1ull << 30;
    constexpr uint32_t count = 64u << 20; // 256 MB of uint
    constexpr uint32_t stride = 1024;     // one write per 4 KB page
    constexpr uint64_t bufferSize = uint64_t(count) * 4;
    slag::MemoryBlock* block =
        device->createMemoryBlock({.size = blockSize, .kind = slag::MemoryKind::GpuOnly, .debugName = "doomed block"})
            .value();
    slag::Buffer* buffer =
        block ? device
                    ->createBuffer({.size = bufferSize,
                                    .placement = slag::Placement::placed(block, blockSize - bufferSize),
                                    .debugName = "freed buffer"})
                    .value()
              : nullptr;
    slag::Semaphore* semaphore = device->createSemaphore({.debugName = "crash done"}).value();
    slag::CommandBuffer* commandBuffer =
        device->createCommandBuffer({.queueType = slag::CommandQueueType::Compute, .debugName = "crash commands"})
            .value();
    if (!pipeline || !buffer || !semaphore || !commandBuffer)
    {
        return 1;
    }
    device->writeRWStructuredBuffer(0, {.buffer = buffer, .count = count, .stride = 4});
    // The descriptor now points at memory that is no longer there.
    device->destroy(buffer);
    device->destroy(block);

    commandBuffer->begin();
    commandBuffer->beginLabel("crash sample");
    commandBuffer->insertLabel("writing to freed memory");
    commandBuffer->bindPipeline(pipeline);
    const uint32_t push[] = {0, count, stride};
    commandBuffer->pushConstants(push, sizeof(push));
    commandBuffer->dispatch(count / stride / 64, 1, 1);
    commandBuffer->endLabel();
    commandBuffer->end();
    slag::SemaphoreValue done = {semaphore, 1};
    slag::CommandBuffer* list[] = {commandBuffer};
    slag::Result<void> submitted =
        device->commandQueue(slag::CommandQueueType::Compute)->submit({.commandBuffers = list, .signals = {&done, 1}});
    slag::Status status = submitted.status();
    if (submitted)
    {
        status = done.semaphore->wait(done.value, 20'000'000'000).status();
    }
    std::printf("wait: %s, device status: %s\n", statusName(status), statusName(device->status()));

    bool lost = device->status() == slag::Status::DeviceLost;
    device->destroy(commandBuffer);
    device->destroy(semaphore);
    device->destroy(pipeline);
    device->shutdown();
    slag::destroy(instance.value());
    std::printf("device lost: %s, crash report: %s\n", lost ? "yes" : "no", messages.crashReport ? "yes" : "no");
    return lost && messages.crashReport ? 0 : 1;
}
