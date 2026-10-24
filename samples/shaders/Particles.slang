// samples/multi_queue: draws each particle as a small additive quad, colored
// by the texture the copy queue uploads.

struct Particle
{
    float2 position;
    float2 velocity;
};

struct PushConstants
{
    uint particles; // StructuredBuffer<Particle>
    uint texture;
    uint sampler;
    float aspect; // height / width
    float2 size;  // quad size in clip space
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 local : TEXCOORD0;
    float3 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex : SV_VertexID, uint instance : SV_InstanceID)
{
    const uint corners[6] = { 0, 1, 2, 0, 2, 3 };
    uint corner = corners[vertex];
    float2 local = float2(corner == 1 || corner == 2 ? 1.0 : -1.0, corner >= 2 ? -1.0 : 1.0);

    StructuredBuffer<Particle> particles = ResourceDescriptorHeap[g_push.particles];
    Texture2D<float4> texture = ResourceDescriptorHeap[g_push.texture];
    SamplerState sampler = SamplerDescriptorHeap[g_push.sampler];
    Particle particle = particles[instance];
    float2 uv = particle.position * float2(0.5, -0.5) + 0.5;

    VertexOutput output;
    float2 center = float2(particle.position.x * g_push.aspect, particle.position.y);
    output.position = float4(center + local * g_push.size, 0.0, 1.0);
    output.local = local;
    output.color = texture.SampleLevel(sampler, uv, 0.0).rgb * (0.25 + length(particle.velocity) * 0.5);
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    float falloff = saturate(1.0 - dot(input.local, input.local));
    return float4(input.color * falloff * 0.35, 1.0);
}
