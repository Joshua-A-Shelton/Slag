public struct ViewData
{
    public matrix<float,4,4> projection;
    public matrix<float,4,4> view;
    public matrix<float,4,4> projectionView;
}
public struct InstanceData
{
    public matrix<float,4,4> transform;
}
struct FragmentParameters
{
    float2 uv;
}
struct VertexStageOutput
{
    FragmentParameters fragmentParameters: FRAMENT_PARAMETERS;
    float4 sv_position: SV_Position;
}

uniform DescriptorHandle<StructuredBuffer<ViewData>> viewData;
uniform DescriptorHandle<StructuredBuffer<InstanceData>> instancePosition;
uniform DescriptorHandle<Texture2D> instanceTexture;
uniform DescriptorHandle<SamplerState> sampler;

[shader("vertex")]
VertexStageOutput vertexMain(float3 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    var camera = getDescriptorFromHandle(viewData);
    var instance = getDescriptorFromHandle(instancePosition);

    float3 worldPosition = mul(instance[0].transform,float4(POSITION,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = UV_COORDINATES;
    output.sv_position = mul(camera[0].projectionView, float4(worldPosition,1));

    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    var texture = getDescriptorFromHandle(instanceTexture);
    var texsampler = getDescriptorFromHandle(sampler);

    return texture.Sample(texsampler,fragmentParameters.uv);
}