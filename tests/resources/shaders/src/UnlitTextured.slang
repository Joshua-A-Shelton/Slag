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
public struct MaterialSamplers
{
    SamplerState sampler;
}

public struct FlatTextureMaterial
{
    float4 color;
    Texture2D sampledTexture;

}
public ParameterBlock<MaterialSamplers> ObjectSampler;
public ParameterBlock<FlatTextureMaterial> FlatMaterialInstance;
public ParameterBlock<ObjectInstanceData> ObjectInstance;

[shader("vertex")]
VertexStageOutput vertexMain(float3 POSITION: POSITION, float2 UV_COORDINATES: UV_COORDINATES)
{
    float3 worldPosition = mul(ObjectInstance.transform,float4(POSITION,1)).xyz;

    VertexStageOutput output;
    output.fragmentParameters.uv = UV_COORDINATES;
    output.sv_position = mul(Camera.projectionView, float4(worldPosition,1));
    return output;
}

[shader("fragment")]
float4 fragmentMain(FragmentParameters fragmentParameters : FragmentParameters) : SV_Target
{
    return FlatMaterialInstance.sampledTexture.Sample(ObjectSampler.sampler,fragmentParameters.uv) * FlatMaterialInstance.color;
}