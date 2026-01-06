//Vertex Position 3D attribute
public struct Vertex2D
{
    public float2 position;
}
//Vertex UV coordinate attribute
public struct UVCoordinates
{
    public float2 position;
}

public struct FragmentParameters
{
    public float4 position: SV_Position;
    public float2 uv_coords: UV_COORDINATES;
}
public struct SamplerParam
{
    public SamplerState sampler;
}
public struct TextureParam
{
    public Texture2D texture;
}

public ParameterBlock<SamplerParam> ColorSampler;
public ParameterBlock<TextureParam> ColorTexture;

[shader("vertex")]
FragmentParameters vertexMain(Vertex2D vertex2d: POSITION, UVCoordinates uvCoordinates: UV_COORDINATES): FragmentParameters
{
    FragmentParameters fragParams;
    fragParams.position = float4(vertex2d.position.x,vertex2d.position.y,0,0);
    fragParams.uv_coords = uvCoordinates.position;
    return fragParams;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return ColorTexture.texture.Sample(ColorSampler.sampler,fragmentParameters.uv_coords);
}