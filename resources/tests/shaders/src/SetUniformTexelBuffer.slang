public struct FragmentParameters
{
    public float2 uv_coords;
}

public struct VertexOutput
{
    public float4 position: SV_Position;
    public FragmentParameters fragmentParamters;
}

public struct TextureBufferParam
{
    [vk::image_format("rgba32f")]
    public Buffer<float4> uniformTexelBuffer;
}

public ParameterBlock<TextureBufferParam> ColorTextureBuffer;

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
    var beginning = ColorTextureBuffer.uniformTexelBuffer.Load(0);
    var end = ColorTextureBuffer.uniformTexelBuffer.Load(1);
    return lerp(beginning,end,fragmentParameters.uv_coords.x);
}