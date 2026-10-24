// samples/bindless: a grid of instanced quads, each sampling the texture whose
// heap index the per-instance table gives it.

struct PushConstants
{
    uint instances; // StructuredBuffer<uint>: a texture index per quad
    uint sampler;
    float aspect; // height / width
    float time;
    uint columns;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    nointerpolation uint texture : TEXTURE;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex : SV_VertexID, uint instance : SV_InstanceID)
{
    // Two triangles: corners 0 1 2, 0 2 3.
    const uint corners[6] = { 0, 1, 2, 0, 2, 3 };
    uint corner = corners[vertex];
    float2 uv = float2(corner == 1 || corner == 2 ? 1.0 : 0.0, corner >= 2 ? 1.0 : 0.0);

    uint column = instance % g_push.columns;
    uint row = instance / g_push.columns;
    float cell = 2.0 / float(g_push.columns);
    float wobble = 0.15 * sin(g_push.time * 2.0 + float(instance) * 0.37);
    float size = cell * (0.8 + wobble);
    float2 center = float2(-1.0 + (float(column) + 0.5) * cell, 1.0 - (float(row) + 0.5) * cell);
    float2 position = center + (uv - 0.5) * float2(size, -size);

    StructuredBuffer<uint> instances = ResourceDescriptorHeap[g_push.instances];
    VertexOutput output;
    output.position = float4(position.x * g_push.aspect, position.y, 0.0, 1.0);
    output.uv = uv;
    output.texture = instances[instance];
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    // The index varies within a draw, so it is marked non-uniform.
    Texture2D<float4> texture = ResourceDescriptorHeap[NonUniformResourceIndex(input.texture)];
    SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
    return texture.Sample(sampler, input.uv);
}
