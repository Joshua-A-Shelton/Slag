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
        if (block->evacuating || block->tlsf.freeBytes() < placement.size)
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

void SubAllocator::setOwner(const PoolAllocation& allocation, void* owner, bool isTexture)
{
    // Find the pool by the block's kind; the node array may move under other allocations.
    size_t index = static_cast<size_t>(allocation.block->memoryBlock->kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    allocation.block->tlsf.setOwner(allocation.node, owner, isTexture);
}

namespace {

uint64_t usedBytes(const PoolBlock* block)
{
    return block->memoryBlock->size - block->tlsf.freeBytes();
}

// Whether every allocation in the block may move (and has its owner yet).
bool fullyMovable(const PoolBlock* block)
{
    for (const Tlsf::Node& node : block->tlsf.nodes())
    {
        if (!node.unused && !node.isFree && (!node.movable || !node.owner))
        {
            return false;
        }
    }
    return true;
}

void sortBlocks(Vector<PoolBlock*>& blocks, bool ascending)
{
    // Insertion sort: pools hold few blocks.
    for (size_t i = 1; i < blocks.size(); ++i)
    {
        PoolBlock* block = blocks[i];
        size_t j = i;
        while (j > 0 &&
               (ascending ? usedBytes(blocks[j - 1]) > usedBytes(block) : usedBytes(blocks[j - 1]) < usedBytes(block)))
        {
            blocks[j] = blocks[j - 1];
            --j;
        }
        blocks[j] = block;
    }
}

} // namespace

Status SubAllocator::planDefragmentation(MemoryKind kind, uint64_t maxBytes, uint32_t maxMoves,
                                         Vector<PlannedMove>& moves)
{
    size_t index = static_cast<size_t>(kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    const HostAllocator* allocator = &device->instance->allocator;
    Vector<PoolBlock*>& pool = pools[index];
    Vector<PoolBlock*> sources(allocator);
    Vector<PoolBlock*> destinations(allocator);
    if (!sources.reserve(pool.size()) || !destinations.reserve(pool.size()))
    {
        return Status::OutOfHostMemory;
    }
    for (PoolBlock* block : pool)
    {
        if (block->tlsf.empty())
        {
            continue; // moving into an empty block frees nothing
        }
        (void)destinations.pushBack(block);
        if (fullyMovable(block))
        {
            (void)sources.pushBack(block);
        }
    }
    sortBlocks(sources, true);       // emptiest first
    sortBlocks(destinations, false); // fullest first

    uint64_t bytes = 0;
    Vector<bool> received(allocator); // per destination: took a move, so it can't be emptied
    if (!received.reserve(destinations.size()))
    {
        return Status::OutOfHostMemory;
    }
    for (size_t i = 0; i < destinations.size(); ++i)
    {
        (void)received.pushBack(false);
    }
    for (PoolBlock* source : sources)
    {
        size_t receivedIndex = 0;
        for (size_t i = 0; i < destinations.size(); ++i)
        {
            receivedIndex = destinations[i] == source ? i : receivedIndex;
        }
        if (received[receivedIndex])
        {
            continue;
        }
        bool partial = moves.empty(); // only the first block may be split, and only by the budget
        size_t firstMove = moves.size();
        uint64_t firstBytes = bytes;
        bool complete = true;
        bool budgetHit = false;
        const Vector<Tlsf::Node>& nodes = source->tlsf.nodes();
        for (uint32_t node = 0; node < nodes.size(); ++node)
        {
            const Tlsf::Node& from = nodes[node];
            if (from.unused || from.isFree)
            {
                continue;
            }
            if ((maxBytes && bytes + from.size > maxBytes) || (maxMoves && moves.size() + 1 > maxMoves))
            {
                complete = false;
                budgetHit = true;
                break;
            }
            PlannedMove move;
            move.owner = from.owner;
            move.ownerIsTexture = from.ownerIsTexture;
            move.from = {source, node};
            move.size = from.size;
            bool placed = false;
            for (size_t i = 0; i < destinations.size() && !placed; ++i)
            {
                PoolBlock* destination = destinations[i];
                if (destination == source || destination->evacuating || destination->tlsf.freeBytes() < from.size)
                {
                    continue;
                }
                Status status =
                    destination->tlsf.allocate(from.size, from.alignment, nullptr, true, move.to.node, move.offset);
                if (status == Status::OutOfHostMemory)
                {
                    return status;
                }
                if (status == Status::Ok)
                {
                    move.to.block = destination;
                    move.block = destination->memoryBlock;
                    received[i] = true;
                    placed = true;
                }
            }
            if (!placed || !moves.pushBack(move))
            {
                if (placed)
                {
                    move.to.block->tlsf.free(move.to.node);
                }
                complete = false;
                break;
            }
            bytes += from.size;
        }
        if (complete)
        {
            source->evacuating = true;
            continue;
        }
        if (!partial || !budgetHit || moves.size() == firstMove)
        {
            // Undo this block's moves: it wouldn't end up empty.
            while (moves.size() > firstMove)
            {
                PlannedMove& undo = moves[moves.size() - 1];
                undo.to.block->tlsf.free(undo.to.node);
                moves.popBack();
            }
            bytes = firstBytes;
        }
        else
        {
            source->evacuating = true; // partially: keep it from refilling
        }
        break; // the budget or the free space is used up
    }
    return Status::Ok;
}

void SubAllocator::endDefragmentation(MemoryKind kind)
{
    size_t index = static_cast<size_t>(kind);
    std::lock_guard<std::mutex> lock(locks[index]);
    for (PoolBlock* block : pools[index])
    {
        block->evacuating = false;
    }
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
