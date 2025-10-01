#include "Core/Mesh.slang"

struct FragmentParameters
{
    float2 uv;
}
struct VertexStageOutput
{
    FragmentParameters fragmentParameters: FRAGMENT_PARAMETERS;
    float4 sv_position: SV_Position;
}

public struct FlatTextureMaterial
{
    float4 color;
    Texture2D sampledTexture;
    SamplerState sampler;
}

public ParameterBlock<FlatTextureMaterial> FlatMaterialInstance;
public ParameterBlock<ObjectInstanceData> ObjectInstance;

[shader("vertex")]
VertexStageOutput vertexMain(Vertex3D vertex3d: POSITION, UVCoordinates uvCoordinates: UV_COORDINATES)
{
    float3 worldPosition = mul(ObjectInstance.transform,float4(vertex3d.position,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = uvCoordinates.position;
    output.sv_position = mul(Camera.projectionView, float4(worldPosition,1));
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return FlatMaterialInstance.sampledTexture.Sample(FlatMaterialInstance.sampler,fragmentParameters.uv) * FlatMaterialInstance.color;
}