// Writes id * 2 + 1 into a RWStructuredBuffer reached through the resource
// heap. Used by the M1 compute-and-readback spike (docs/milestones.md).

struct PushConstants
{
    uint bufferIndex;
    uint count;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_push.count)
        return;
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[g_push.bufferIndex];
    buffer[id.x] = id.x * 2 + 1;
}
