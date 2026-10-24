// samples/compute: a full-screen triangle sampling a texture, letterboxed to
// a square.

struct PushConstants
{
    uint texture;
    uint sampler;
    float aspect; // height / width
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex : SV_VertexID)
{
    float2 position = float2((vertex << 1) & 2, vertex & 2) * 2.0 - 1.0;
    VertexOutput output;
    output.position = float4(position, 0.0, 1.0);
    // Square texels: stretch the shorter axis's coordinate range.
    float2 scale = g_push.aspect < 1.0 ? float2(1.0 / g_push.aspect, 1.0) : float2(1.0, g_push.aspect);
    output.uv = (float2(position.x, -position.y) * scale) * 0.5 + 0.5;
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    if (any(input.uv < 0.0) || any(input.uv > 1.0))
    {
        return float4(0.02, 0.02, 0.04, 1.0);
    }
    Texture2D<float4> texture = ResourceDescriptorHeap[g_push.texture];
    SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
    return texture.Sample(sampler, input.uv);
}
