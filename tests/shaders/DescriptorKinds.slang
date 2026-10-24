// Reads one value through every descriptor kind and writes them to an output
// buffer; writes one texel of a RWTexture2D. Used by DescriptorTests.cpp.

struct PushConstants
{
    uint constants;
    uint raw;
    uint structured;
    uint typed;
    uint texture;
    uint sampler;
    uint rwTexture;
    uint output;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct Constants
{
    float4 value;
};

[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWStructuredBuffer<float4> output = ResourceDescriptorHeap[g_push.output];
    ConstantBuffer<Constants> constants = ResourceDescriptorHeap[g_push.constants];
    ByteAddressBuffer raw = ResourceDescriptorHeap[g_push.raw];
    StructuredBuffer<float4> structured = ResourceDescriptorHeap[g_push.structured];
    Buffer<float4> typed = ResourceDescriptorHeap[g_push.typed];
    Texture2D<float4> texture = ResourceDescriptorHeap[g_push.texture];
    SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
    RWTexture2D<float4> rwTexture = ResourceDescriptorHeap[g_push.rwTexture];

    output[0] = constants.value;
    output[1] = asfloat(raw.Load4(16));
    output[2] = structured[1];
    output[3] = typed[2];
    output[4] = texture.SampleLevel(sampler, float2(2.5 / 4.0, 1.5 / 4.0), 0); // texel (2, 1), point sampled
    output[5] = texture.Load(int3(3, 2, 0));
    rwTexture[int2(1, 1)] = float4(1.0, 0.0, 1.0, 1.0);
}
