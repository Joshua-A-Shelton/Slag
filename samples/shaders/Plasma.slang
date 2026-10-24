// samples/compute: animates a plasma pattern into a RWTexture2D.

struct PushConstants
{
    uint texture;
    uint size;
    float time;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_push.size || id.y >= g_push.size)
    {
        return;
    }
    RWTexture2D<float4> output = ResourceDescriptorHeap[g_push.texture];
    float2 p = float2(id.xy) / float(g_push.size) * 8.0;
    float t = g_push.time;
    float v = sin(p.x + t) + sin(p.y * 0.7 + t * 1.3) + sin((p.x + p.y) * 0.5 + t * 0.7) +
              sin(length(p - 4.0) * 1.5 - t * 2.0);
    float3 color = 0.5 + 0.5 * cos(v * 1.2 + float3(0.0, 2.1, 4.2));
    output[id.xy] = float4(color, 1.0);
}
