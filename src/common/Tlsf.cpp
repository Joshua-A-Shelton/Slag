#include "common/Tlsf.h"

#include <bit>

namespace slag {

namespace {

uint64_t alignUp(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace

Tlsf::Tlsf(const HostAllocator* allocator)
    : nodeArray(allocator)
    , unusedNodes(allocator)
{
    for (auto& row : heads)
    {
        for (uint32_t& head : row)
        {
            head = invalid;
        }
    }
}

bool Tlsf::init(uint64_t size)
{
    uint32_t node = invalid;
    if (!reserveNodes(1))
    {
        return false;
    }
    newNode(node);
    nodeArray[node].offset = 0;
    nodeArray[node].size = size / granularity * granularity;
    freeSize = nodeArray[node].size;
    insertFree(node);
    return true;
}

// Sizes in granularity units: below 32 units one list per size, then 32 lists
// per power of two.
void Tlsf::mapping(uint64_t size, uint32_t& firstLevel, uint32_t& secondLevel)
{
    uint64_t units = size / granularity;
    if (units < secondLevelCount)
    {
        firstLevel = 0;
        secondLevel = static_cast<uint32_t>(units);
        return;
    }
    uint32_t log2 = static_cast<uint32_t>(std::bit_width(units)) - 1;
    firstLevel = log2 - secondLevelBits + 1;
    secondLevel = static_cast<uint32_t>((units >> (log2 - secondLevelBits)) & (secondLevelCount - 1));
}

// A free node of at least `size` bytes, or invalid. Rounds up to the next list
// so any node in the list found is big enough.
uint32_t Tlsf::findFree(uint64_t size) const
{
    uint64_t units = size / granularity;
    if (units >= secondLevelCount)
    {
        uint32_t log2 = static_cast<uint32_t>(std::bit_width(units)) - 1;
        units += (uint64_t(1) << (log2 - secondLevelBits)) - 1;
    }
    uint32_t firstLevel = 0;
    uint32_t secondLevel = 0;
    mapping(units * granularity, firstLevel, secondLevel);
    if (firstLevel >= firstLevelCount)
    {
        return invalid;
    }

    uint32_t secondMap = secondLevelMap[firstLevel] & (~0u << secondLevel);
    if (secondMap == 0)
    {
        uint64_t firstMap = firstLevel + 1 < 64 ? firstLevelMap & (~uint64_t(0) << (firstLevel + 1)) : 0;
        if (firstMap == 0)
        {
            return invalid;
        }
        firstLevel = static_cast<uint32_t>(std::countr_zero(firstMap));
        secondMap = secondLevelMap[firstLevel];
    }
    return heads[firstLevel][std::countr_zero(secondMap)];
}

Status Tlsf::allocate(uint64_t size, uint64_t alignment, void* owner, bool movable, uint32_t& node, uint64_t& offset)
{
    size = alignUp(size == 0 ? 1 : size, granularity);
    alignment = alignment < granularity ? granularity : alignment;

    // Nodes are mostly aligned already: try a node of `size` first, then one
    // big enough for any padding.
    uint32_t candidate = findFree(size);
    if (candidate != invalid && alignUp(nodeArray[candidate].offset, alignment) + size >
                                    nodeArray[candidate].offset + nodeArray[candidate].size)
    {
        candidate = findFree(size + alignment - granularity);
    }
    if (candidate == invalid)
    {
        return Status::Unsupported;
    }
    // Up to two new nodes: padding before, remainder after.
    if (!reserveNodes(2))
    {
        return Status::OutOfHostMemory;
    }

    removeFree(candidate);
    Node* current = &nodeArray[candidate];
    uint64_t padding = alignUp(current->offset, alignment) - current->offset;
    if (padding > 0)
    {
        // The node before a free node is never free (free nodes coalesce).
        uint32_t before = invalid;
        newNode(before);
        current = &nodeArray[candidate];
        Node& front = nodeArray[before];
        front.offset = current->offset;
        front.size = padding;
        front.prevPhysical = current->prevPhysical;
        front.nextPhysical = candidate;
        if (current->prevPhysical != invalid)
        {
            nodeArray[current->prevPhysical].nextPhysical = before;
        }
        current->prevPhysical = before;
        current->offset += padding;
        current->size -= padding;
        insertFree(before);
    }
    uint64_t remainder = current->size - size;
    if (remainder > 0)
    {
        uint32_t after = invalid;
        newNode(after);
        current = &nodeArray[candidate];
        Node& back = nodeArray[after];
        back.offset = current->offset + size;
        back.size = remainder;
        back.prevPhysical = candidate;
        back.nextPhysical = current->nextPhysical;
        if (current->nextPhysical != invalid)
        {
            nodeArray[current->nextPhysical].prevPhysical = after;
        }
        current->nextPhysical = after;
        current->size = size;
        insertFree(after);
    }

    current->isFree = false;
    current->owner = owner;
    current->movable = movable;
    current->alignment = alignment;
    freeSize -= size;
    ++allocationCount;
    node = candidate;
    offset = current->offset;
    return Status::Ok;
}

void Tlsf::free(uint32_t index)
{
    Node* node = &nodeArray[index];
    node->isFree = true;
    node->owner = nullptr;
    node->ownerIsTexture = false;
    node->movable = false;
    node->alignment = 0;
    freeSize += node->size;
    --allocationCount;

    uint32_t next = node->nextPhysical;
    if (next != invalid && nodeArray[next].isFree)
    {
        removeFree(next);
        node->size += nodeArray[next].size;
        node->nextPhysical = nodeArray[next].nextPhysical;
        if (node->nextPhysical != invalid)
        {
            nodeArray[node->nextPhysical].prevPhysical = index;
        }
        releaseNode(next);
        node = &nodeArray[index];
    }
    uint32_t previous = node->prevPhysical;
    if (previous != invalid && nodeArray[previous].isFree)
    {
        removeFree(previous);
        Node& merged = nodeArray[previous];
        merged.size += node->size;
        merged.nextPhysical = node->nextPhysical;
        if (merged.nextPhysical != invalid)
        {
            nodeArray[merged.nextPhysical].prevPhysical = previous;
        }
        releaseNode(index);
        index = previous;
    }
    insertFree(index);
}

void Tlsf::insertFree(uint32_t index)
{
    Node& node = nodeArray[index];
    node.isFree = true;
    uint32_t firstLevel = 0;
    uint32_t secondLevel = 0;
    mapping(node.size, firstLevel, secondLevel);
    node.prevFree = invalid;
    node.nextFree = heads[firstLevel][secondLevel];
    if (node.nextFree != invalid)
    {
        nodeArray[node.nextFree].prevFree = index;
    }
    heads[firstLevel][secondLevel] = index;
    firstLevelMap |= uint64_t(1) << firstLevel;
    secondLevelMap[firstLevel] |= 1u << secondLevel;
}

void Tlsf::removeFree(uint32_t index)
{
    Node& node = nodeArray[index];
    uint32_t firstLevel = 0;
    uint32_t secondLevel = 0;
    mapping(node.size, firstLevel, secondLevel);
    if (node.prevFree != invalid)
    {
        nodeArray[node.prevFree].nextFree = node.nextFree;
    }
    else
    {
        heads[firstLevel][secondLevel] = node.nextFree;
    }
    if (node.nextFree != invalid)
    {
        nodeArray[node.nextFree].prevFree = node.prevFree;
    }
    if (heads[firstLevel][secondLevel] == invalid)
    {
        secondLevelMap[firstLevel] &= ~(1u << secondLevel);
        if (secondLevelMap[firstLevel] == 0)
        {
            firstLevelMap &= ~(uint64_t(1) << firstLevel);
        }
    }
    node.prevFree = invalid;
    node.nextFree = invalid;
}

// Room for `count` more nodes, so newNode and releaseNode can't fail.
bool Tlsf::reserveNodes(size_t count)
{
    size_t needed = nodeArray.size() + count;
    return nodeArray.reserve(needed) && unusedNodes.reserve(needed);
}

void Tlsf::newNode(uint32_t& node)
{
    if (!unusedNodes.empty())
    {
        node = unusedNodes[unusedNodes.size() - 1];
        unusedNodes.popBack();
        nodeArray[node] = Node{};
        return;
    }
    (void)nodeArray.pushBack(Node{}); // reserved by reserveNodes
    node = static_cast<uint32_t>(nodeArray.size() - 1);
}

void Tlsf::releaseNode(uint32_t node)
{
    nodeArray[node] = Node{};
    nodeArray[node].unused = true;
    (void)unusedNodes.pushBack(node); // capacity covers every node
}

} // namespace slag
