// Copies the root constant buffer (register b1, ADR 0044) into an output
// buffer. Used by DescriptorTests.cpp.

struct PushConstants
{
    uint output;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct Frame
{
    float4 first;
    uint4 second;
};

ConstantBuffer<Frame> g_frame : register(b1);

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWStructuredBuffer<float4> output = ResourceDescriptorHeap[g_push.output];
    output[0] = g_frame.first;
    output[1] = asfloat(g_frame.second);
}
