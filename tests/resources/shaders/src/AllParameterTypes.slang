public struct GroupOne
{
    public float4 colorUniformBuffer;
    public SamplerState sampler;
    public Texture2D sampledTexture;
    public WTexture2D storageTexture;
}
public struct mything
{
    public float mything1;
    public int mything2;
}
public struct GroupTwo
{
    [vk::image_format("rgba32f")]
    public Buffer<float4> uniformTexelBuffer;
    public RWBuffer<uint4> storageTexelBuffer[2];
    public StructuredBuffer<mything> storageBuffer;
}


//Vertex Position 3D attribute
public struct Vertex3D
{
    public float3 position;
}
//Vertex UV coordinate attribute
public struct UVCoordinates
{
    public float2 position;
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

public ParameterBlock<GroupOne> First;
public ParameterBlock<GroupTwo> Second;

[shader("vertex")]
VertexStageOutput vertexMain(Vertex3D vertex3d, UVCoordinates uvCoordinates)
{
    VertexStageOutput output;
    output.fragmentParameters.uv = uvCoordinates.position;
    output.sv_position = float4(vertex3d.position,1);
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    var color = First.colorUniformBuffer * First.sampledTexture.Sample(First.sampler,fragmentParameters.uv);
    var count = Second.storageBuffer.getCount();
    var read = Second.uniformTexelBuffer.Sample(First.sampler,12.0f);
    if(count < 1)
    {
        First.storageTexture.Store(uint2(0,0),color);
        Second.storageTexelBuffer[0].Store(0,uint4((uint)(read.x*255),(uint)(read.y*255),(uint)(read.z*255),(uint)(read.w*255)));
    }

    return color;
}