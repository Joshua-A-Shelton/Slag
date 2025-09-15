//Vertex Position 3D attribute
public struct Vertex3D
{
    public float3 position;
}
//Vertex Position 2D attribute
public struct Vertex2D
{
    public float2 position;
}
//Vertex Normal attribute
public struct Normal
{
    public float3 direction;
}
public struct Tangent
{
    public float3 direction;
}
//Vertex UV coordinate attribute
public struct UVCoordinates
{
    public float2 position;
}
//Vertex color attribute
public struct VertexColor
{
    public uint8_t4 values;
}
public struct BoneIndexWeightValue
{
    public uint16_t boneId;
    public float weight;
}
//Vertex Bone Weights attribute
public struct BoneWeights
{
    public BoneIndexWeightValue[4] weights;
}

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

public struct InstanceData
{
    public matrix<float,4,4> transform;
    public Texture2D sampledTexture;
    public SamplerState sampler;
}

struct VertexStageOutput
{
    FragmentParameters fragmentParameters;
    float4 sv_position: SV_Position;
}

struct FragmentParameters
{
    float2 uv;
}

public ParameterBlock<GlobalsData> Globals;
public ParameterBlock<InstanceData> Instance;


[shader("vertex")]
VertexStageOutput vertexMain(Vertex3D vertex3d, UVCoordinates uvCoordinates)
{
    float3 worldPosition = mul(Instance.transform,float4(vertex3d.position,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = uvCoordinates.position;
    output.sv_position = mul(Globals.projectionView, float4(worldPosition,1));
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return Instance.sampledTexture.Sample(Instance.sampler,fragmentParameters.uv);
}