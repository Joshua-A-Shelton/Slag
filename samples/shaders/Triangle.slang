// samples/triangle: a vertex-colored triangle from a vertex buffer.

struct VertexInput
{
    float2 position : POSITION;
    float3 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}

[shader("pixel")]
float4 pixelMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
