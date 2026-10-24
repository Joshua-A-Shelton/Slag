// A full-screen triangle at a fixed depth, with no pixel shader needed.
// Used by RenderTests.cpp for depth/stencil passes.

[shader("vertex")]
float4 vertexMain(uint vertex : SV_VertexID) : SV_Position
{
    float2 position = float2((vertex << 1) & 2, vertex & 2) * 2.0 - 1.0;
    return float4(position, 0.25, 1.0);
}
