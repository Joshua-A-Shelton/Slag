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
FragmentParameters vertexMain(float2 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES): FragmentParameters
{
    FragmentParameters fragParams;
    fragParams.position = float4(POSITION.x,POSITION.y,0,0);
    fragParams.uv_coords = UV_COORDINATES;
    return fragParams;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return ColorTexture.texture.Sample(ColorSampler.sampler,fragmentParameters.uv_coords);
}