//Data that is set once per view (usually camera) per frame
public struct ViewData
{
    public matrix<float,4,4> projection;
    public matrix<float,4,4> view;
    public matrix<float,4,4> projectionView;
}

public struct GlobalsData
{
    public matrix<float,4,4> projection;
    public matrix<float,4,4> view;
    public matrix<float,4,4> projectionView;
}
public struct InstanceSampler
{
    public SamplerState sampler;
}
public struct InstanceData
{
    public matrix<float,4,4> transform;
    public Texture2D sampledTexture;
}

struct VertexStageOutput
{
    FragmentParameters fragmentParameters: FRAMENT_PARAMETERS;
    float4 sv_position: SV_Position;
}

struct FragmentParameters
{
    float2 uv;
}

public ParameterBlock<GlobalsData> Globals;
public ParameterBlock<InstanceSampler> Sampler;
public ParameterBlock<InstanceData> Instance;


[shader("vertex")]
VertexStageOutput vertexMain(float3 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    float3 worldPosition = mul(Instance.transform,float4(POSITION,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = UV_COORDINATES;
    output.sv_position = mul(Globals.projectionView, float4(worldPosition,1));
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return Instance.sampledTexture.Sample(Sampler.sampler,fragmentParameters.uv);
}