public struct PushConstants
{
    public float2 position;
    public float2 scale;
    public float4 color;
}
[vk::push_constant]
PushConstants pushConstants;

public struct Globals
{
    float4x4 projectionMatrix;
}
ParameterBlock<Globals> globals;

[shader("vertex")]
float4 vertexMain(float3 vertex: VERTEX_POSITION): SV_Position
{
    var modified = vertex + float3(pushConstants.position,0);
    modified.x*= pushConstants.scale.x;
    modified.y*=pushConstants.scale.y;
    return mul(globals.projectionMatrix,float4(modified,1));
}
[shader("fragment")]
float4 fragmentMain(): SV_Target
{
    return pushConstants.color;
}