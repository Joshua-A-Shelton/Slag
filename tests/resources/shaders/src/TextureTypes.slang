public struct GroupOne
{
    public int index;
    public SamplerState sampler;
    public Texture1D[3] texture1D;
    public Texture1DArray texture1DArray;
    public texture2D[3] texture2D;
    public Texture2DArray texture2DArray;
    public Texture3D[3] texture3D;
    public TextureCube[3] textureCube;
    public TextureCubeArray textureCubeArray;
}
public struct Vertex3D
{
    public float3 position;
}

public struct UVCoordinates
{
    public float2 position;
}

struct FragmentParameters
{
    float2 uv;
}

struct VertexStageOutput
{
    FragmentParameters fragmentParameters: FRAGMENT_PARAMETERS;
    float4 sv_position: SV_Position;
}

public ParameterBlock<GroupOne> First;

[shader("vertex")]
VertexStageOutput vertexMain(Vertex3D vertex3d: POSITION, UVCoordinates uvCoordinates: UV_COORDINATES)
{
    VertexStageOutput output;
    output.fragmentParameters.uv = uvCoordinates.position;
    output.sv_position = float4(vertex3d.position,1);
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    var color1 = First.texture1D[First.index].Sample(First.sampler,.5);
    var color2 = First.texture1DArray.Sample(First.sampler,float2(.5,.5));
    var color3 = First.texture2D[First.index].Sample(First.sampler,fragmentParameters.uv);
    var color4 = First.texture2DArray.Sample(First.sampler,float3(fragmentParameters.uv,.7));
    var color5 = First.texture3D[First.index].Sample(First.sampler,float3(0,.5,.2));
    var color6 = First.textureCube[First.index].Sample(First.sampler,float3(1,-1,1));
    var color7 = First.textureCubeArray.Sample(First.sampler,float4(1,1,1,.6));
    return color1*color2*color3*color4*color5*color6*color7;

}