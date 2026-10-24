// The M3 scene (SceneTests.cpp): vertex-colored or textured quads, tinted by
// the root constant buffer. Vertices with color alpha 0 are textured.

struct PushConstants
{
    uint texture;
    uint sampler;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct Tint
{
    float4 value;
};

ConstantBuffer<Tint> g_tint : register(b1);

struct VertexInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    if (input.color.a == 0.0)
    {
        Texture2D<float4> texture = ResourceDescriptorHeap[g_push.texture];
        SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
        return float4(texture.Sample(sampler, input.uv).rgb, 1.0) * g_tint.value;
    }
    return input.color * g_tint.value;
}
