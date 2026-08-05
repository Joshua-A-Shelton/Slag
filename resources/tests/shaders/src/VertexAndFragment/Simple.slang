uniform DescriptorHandle<Texture2D> instanceTexture;
uniform DescriptorHandle<SamplerState> sampler;
struct FragmentParameters
{
    float2 uvCoordinates;
}
struct VertexOutput
{
    FragmentParameters fragmentParameters: FRAGMENT_PARAMETERS;
    float4 position: SV_POSITION;
}

[shader("vertex")]
VertexOutput vertexMain(float3 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    VertexOutput output;
    output.position = float4(POSITION,1);
    output.fragmentParameters.uvCoordinates = UV_COORDINATES;
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters: FRAGMENT_PARAMETERS) : SV_Target
{
    return instanceTexture.Sample(sampler,fragmentParameters.uvCoordinates);
}