// samples/multi_queue: moves particles through a swirling flow field on the
// compute queue. The field is deliberately expensive, so the work is long
// enough to measure.

struct Particle
{
    float2 position;
    float2 velocity;
};

struct PushConstants
{
    uint input;  // StructuredBuffer<Particle>
    uint output; // RWStructuredBuffer<Particle>
    uint count;
    float time;
};

[[vk::push_constant]] ConstantBuffer<PushConstants> g_push;

float2 flow(float2 p, float t)
{
    float2 force = float2(-p.y, p.x) * 0.6 - p * 0.15;
    float2 q = p * 3.0;
    for (int i = 0; i < 160; ++i)
    {
        q = float2(sin(q.y * 1.3 + t * 0.3), cos(q.x * 1.1 - t * 0.2)) + p * 2.0;
        force += float2(q.y, -q.x) * 0.01;
    }
    return force;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_push.count)
    {
        return;
    }
    StructuredBuffer<Particle> input = ResourceDescriptorHeap[g_push.input];
    RWStructuredBuffer<Particle> output = ResourceDescriptorHeap[g_push.output];
    Particle particle = input[id.x];
    const float dt = 1.0 / 60.0;
    particle.velocity = particle.velocity * 0.98 + flow(particle.position, g_push.time) * dt;
    particle.position += particle.velocity * dt;
    // Wrap at the edges.
    particle.position = particle.position - 2.0 * floor((particle.position + 1.0) * 0.5);
    output[id.x] = particle;
}
