// Vertex-colored triangles from a vertex buffer (POSITION at location 0,
// COLOR at location 1). Used by RenderTests.cpp.

struct VertexInput
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 0.5, 1.0);
    output.color = input.color;
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    return input.color;
}
