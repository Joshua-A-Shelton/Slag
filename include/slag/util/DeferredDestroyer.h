#pragma once

// Optional utility: destroys objects once a semaphore value is reached (ADR 0069).
// Built only on public Slag functions.

#include <slag/Semaphore.h>

namespace slag {

class AttachmentView;
class Buffer;
class CommandBuffer;
class GraphicsDevice;
class MemoryBlock;
class Pipeline;
class QueryPool;
class Texture;

// Not thread-safe (CLAUDE.md §7): one thread at a time, or one per thread.
// Objects reached in the same collect() are destroyed views first, then
// resources, query pools, pipelines, command buffers, memory blocks and
// semaphores, so a memory block (or a semaphore other entries wait on) is
// passed with a value no earlier than the objects that need it.
class DeferredDestroyer
{
public:
    explicit DeferredDestroyer(GraphicsDevice* device);
    // Calls flush(); destroy it before shutting the device down.
    ~DeferredDestroyer();

    // Destroys `object` once `after` has been reached. Null objects are
    // ignored; an object that can't be recorded (out of host memory) is
    // destroyed after waiting for `after`.
    void destroyAfter(AttachmentView* object, SemaphoreValue after);
    void destroyAfter(Buffer* object, SemaphoreValue after);
    void destroyAfter(Texture* object, SemaphoreValue after);
    void destroyAfter(QueryPool* object, SemaphoreValue after);
    void destroyAfter(Pipeline* object, SemaphoreValue after);
    void destroyAfter(CommandBuffer* object, SemaphoreValue after);
    void destroyAfter(MemoryBlock* object, SemaphoreValue after);
    void destroyAfter(Semaphore* object, SemaphoreValue after);

    // Non-blocking: destroys everything whose semaphore value has been reached.
    void collect();
    // Waits for everything pending, then destroys it (shutdown).
    void flush();

    DeferredDestroyer(const DeferredDestroyer&) = delete;
    DeferredDestroyer& operator=(const DeferredDestroyer&) = delete;

private:
    struct State;
    GraphicsDevice* device = nullptr;
    State* state = nullptr;
};

} // namespace slag
