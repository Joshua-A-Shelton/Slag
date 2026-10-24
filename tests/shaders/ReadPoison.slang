// Reads a raw buffer slot, a float texture slot, and a uint texture slot into
// an output buffer. Used by DescriptorTests.cpp for poison descriptors.

struct PushConstants
{
    uint rawBuffer;
    uint floatTexture;
    uint uintTexture;
    uint output;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    ByteAddressBuffer raw = ResourceDescriptorHeap[g_push.rawBuffer];
    Texture2D<float4> floats = ResourceDescriptorHeap[g_push.floatTexture];
    Texture2D<uint4> uints = ResourceDescriptorHeap[g_push.uintTexture];
    RWStructuredBuffer<uint> output = ResourceDescriptorHeap[g_push.output];
    output[0] = raw.Load(0);
    float4 f = floats.Load(int3(0, 0, 0));
    output[1] = asuint(f.x);
    output[2] = asuint(f.y);
    output[3] = asuint(f.z);
    output[4] = asuint(f.w);
    uint4 u = uints.Load(int3(0, 0, 0));
    output[5] = u.x;
    output[6] = u.w;
}
