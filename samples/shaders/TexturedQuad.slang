// samples/textured_quad: an indexed quad sampling a texture through the heaps,
// spinning by an angle in the root constant buffer.

struct PushConstants
{
    uint texture;
    uint sampler;
    float aspect; // height / width
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct Frame
{
    float angle;
};

ConstantBuffer<Frame> g_frame : register(b1);

struct VertexInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    float s = sin(g_frame.angle);
    float c = cos(g_frame.angle);
    float2 rotated = float2(input.position.x * c - input.position.y * s, input.position.x * s + input.position.y * c);
    VertexOutput output;
    output.position = float4(rotated.x * g_push.aspect, rotated.y, 0.0, 1.0);
    output.uv = input.uv;
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    Texture2D<float4> texture = ResourceDescriptorHeap[g_push.texture];
    SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
    return texture.Sample(sampler, input.uv);
}
