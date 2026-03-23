public struct FragmentParameters
{
    public float2 uv_coords;
}

public struct VertexOutput
{
    public float4 position: SV_Position;
    public FragmentParameters fragmentParamters;
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
VertexOutput vertexMain(float2 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    VertexOutput vo;
    vo.position = float4(POSITION.x,POSITION.y,.5f,1);
    vo.fragmentParamters.uv_coords = UV_COORDINATES;
    return vo;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FRAGMENT_PARAMETERS) : SV_Target
{
    return ColorTexture.texture.Sample(ColorSampler.sampler,fragmentParameters.uv_coords);
}