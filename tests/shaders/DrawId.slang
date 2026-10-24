// Quads placed in a 4x2 grid by draw ID (column) and instance (row), with
// no vertex buffers. The draw ID is the draw's firstInstance, read through
// SV_StartInstanceLocation (ADR 0047). Used by RenderTests.cpp.

struct VertexOutput
{
    float4 position : SV_Position;
    nointerpolation uint2 id : ID;
};

[shader("vertex")]
VertexOutput vertexMain(uint vertex : SV_VertexID, uint instance : SV_InstanceID, uint drawId : SV_StartInstanceLocation)
{
    const float2 corners[6] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 0), float2(1, 1), float2(0, 1) };
    float2 corner = corners[vertex];
    float x = (drawId + corner.x) / 4.0;
    float y = (instance + corner.y) / 2.0;
    VertexOutput output;
    output.position = float4(x * 2.0 - 1.0, 1.0 - y * 2.0, 0.5, 1.0);
    output.id = uint2(drawId, instance);
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    // Exact byte values (k / 255): no rounding ties, which GPUs may break either way.
    return float4((input.id.x + 1) * 32 / 255.0, (input.id.y + 1) * 64 / 255.0, 0.0, 1.0);
}
