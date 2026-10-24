#include "common/SubAllocator.h"

namespace slag {

namespace {

constexpr uint64_t mebibyte = 1024 * 1024;

// 256 MB, or heap size / 8 on heaps of 1 GB or less (ADR 0061); a multiple of
// 4 MB so MSAA textures fit (ADR 0023).
uint64_t preferredBlockSize(uint64_t heapSize)
{
    uint64_t size = heapSize <= 1024 * mebibyte ? heapSize / 8 : 256 * mebibyte;
    size = size / (4 * mebibyte) * (4 * mebibyte);
    return size < 4 * mebibyte ? 4 * mebibyte : size;
}

const char* poolName(MemoryKind kind)
{
    switch (kind)
    {
    case MemoryKind::GpuOnly:
        return "slag GpuOnly pool";
    case MemoryKind::Upload:
        return "slag Upload pool";
    case MemoryKind::Readback:
        return "slag Readback pool";
    case MemoryKind::GpuUpload:
        return "slag GpuUpload pool";
    }
    return "slag pool";
}

} // namespace

SubAllocator::SubAllocator(GraphicsDeviceCommon* device)
    : device(device)
{
    const HostAllocator* allocator = &device->instance->allocator;
    for (Vector<PoolBlock*>& pool : pools)
    {
        pool = Vector<PoolBlock*>(allocator);
    }
    // GPU memory for the GPU-side kinds, system memory for the CPU-side ones.
    uint64_t dedicated = device->info.dedicatedMemoryBytes;
    uint64_t shared = device->info.sharedMemoryBytes ? device->info.sharedMemoryBytes : dedicated;
    blockSizes[static_cast<size_t>(MemoryKind::GpuOnly)] = preferredBlockSize(dedicated);
    blockSizes[static_cast<size_t>(MemoryKind::GpuUpload)] = preferredBlockSize(dedicated);
    blockSizes[static_cast<size_t>(MemoryKind::Upload)] = preferredBlockSize(shared);
    blockSizes[static_cast<size_t>(MemoryKind::Readback)] = preferredBlockSize(shared);
}

SubAllocator::~SubAllocator()
{
    for (Vector<PoolBlock*>& pool : pools)
    {
        for (PoolBlock* block : pool)
        {
            SLAG_DISPATCH(device, destroyMemoryBlock)(device, block->memoryBlock);
            device->instance->allocator.destroy(block);
        }
        pool.clear();
    }
}

Status SubAllocator::allocate(MemoryKind kind, const PlacementInfo& placement, bool movable, PoolAllocation& out,
                              MemoryBlockCommon*& memoryBlock, uint64_t& offset)
{
    size_t index = static_cast<size_t>(kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    Vector<PoolBlock*>& pool = pools[index];
    for (PoolBlock* block : pool)
    {
        if (block->tlsf.freeBytes() < placement.size)
        {
            continue;
        }
        Status status = block->tlsf.allocate(placement.size, placement.alignment, nullptr, movable, out.node, offset);
        if (status == Status::Ok)
        {
            out.block = block;
            memoryBlock = block->memoryBlock;
            return Status::Ok;
        }
        if (status != Status::Unsupported)
        {
            return status;
        }
    }

    // No room: a new block.
    if (!pool.reserve(pool.size() + 1))
    {
        return Status::OutOfHostMemory;
    }
    auto* block = device->instance->allocator.create<PoolBlock>(&device->instance->allocator);
    if (!block || !block->tlsf.init(blockSizes[index]))
    {
        device->instance->allocator.destroy(block);
        return Status::OutOfHostMemory;
    }
    MemoryBlockDesc desc;
    desc.size = blockSizes[index];
    desc.kind = kind;
    desc.debugName = poolName(kind);
    Result<MemoryBlockCommon*> created = SLAG_DISPATCH(device, createMemoryBlock)(device, desc);
    if (!created)
    {
        device->instance->allocator.destroy(block);
        return created.status();
    }
    block->memoryBlock = created.value();
    (void)pool.pushBack(block); // reserved above
    Status status = block->tlsf.allocate(placement.size, placement.alignment, nullptr, movable, out.node, offset);
    if (status == Status::Ok)
    {
        out.block = block;
        memoryBlock = block->memoryBlock;
    }
    return status;
}

void SubAllocator::setOwner(const PoolAllocation& allocation, void* owner)
{
    // Find the pool by the block's kind; the node array may move under other allocations.
    size_t index = static_cast<size_t>(allocation.block->memoryBlock->kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    allocation.block->tlsf.setOwner(allocation.node, owner);
}

void SubAllocator::free(PoolAllocation& allocation)
{
    if (!allocation.block)
    {
        return;
    }
    size_t index = static_cast<size_t>(allocation.block->memoryBlock->kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    PoolBlock* block = allocation.block;
    block->tlsf.free(allocation.node);
    allocation = {};
    if (!block->tlsf.empty())
    {
        return;
    }

    // Keep one empty block per kind to avoid churn (ADR 0061).
    Vector<PoolBlock*>& pool = pools[index];
    size_t emptyBlocks = 0;
    for (PoolBlock* other : pool)
    {
        emptyBlocks += other->tlsf.empty() ? 1 : 0;
    }
    if (emptyBlocks <= 1)
    {
        return;
    }
    for (size_t i = 0; i < pool.size(); ++i)
    {
        if (pool[i] == block)
        {
            pool[i] = pool[pool.size() - 1];
            pool.popBack();
            break;
        }
    }
    SLAG_DISPATCH(device, destroyMemoryBlock)(device, block->memoryBlock);
    device->instance->allocator.destroy(block);
}

} // namespace slag
