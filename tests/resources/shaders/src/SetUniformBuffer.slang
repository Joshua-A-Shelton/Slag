public struct FragmentParameters
{
    public float2 uv_coords;
}

public struct VertexOutput
{
    public float4 position: SV_Position;
    public FragmentParameters fragmentParamters;
}

public struct ParameterData
{
    float4 color1;
    float4 color2;
    float boundary;
}

ParameterBlock<ParameterData> ShaderParameters;

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
    if(fragmentParameters.uv_coords.x < ShaderParameters.boundary)
    {
        return ShaderParameters.color1;
    }
    return ShaderParameters.color2;
}