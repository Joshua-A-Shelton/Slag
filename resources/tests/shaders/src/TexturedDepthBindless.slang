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


uniform DescriptorHandle<ConstantBuffer<ViewData>> viewData;
uniform DescriptorHandle<ConstantBuffer<InstanceData>> instancePosition;
uniform DescriptorHandle<Texture2D> instanceTexture;
uniform DescriptorHandle<SamplerState> sampler;

[shader("vertex")]
VertexStageOutput vertexMain(float3 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    var globData = getDescriptorFromHandle(viewData);

    var instPosition = getDescriptorFromHandle(instancePosition);

    float3 worldPosition = mul(instPosition.transform,float4(POSITION,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = UV_COORDINATES;
    output.sv_position = mul(globData.projectionView, float4(worldPosition,1));

    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return instanceTexture.Sample(sampler,fragmentParameters.uv);
}