// samples/crash: writes through a descriptor whose buffer memory is gone.

struct PushConstants
{
    uint buffer; // RWStructuredBuffer<uint>
    uint count;  // elements in the buffer
    uint stride; // elements between writes: a few writes spread over the whole range
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x * g_push.stride;
    if (index < g_push.count)
    {
        RWStructuredBuffer<uint> output = ResourceDescriptorHeap[g_push.buffer];
        output[index] = id.x;
    }
}
