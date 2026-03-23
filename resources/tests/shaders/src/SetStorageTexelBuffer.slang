[vk::image_format("rgba32f")]
RWBuffer<float4> operands1;
[vk::image_format("rgba32f")]
RWBuffer<float4> operands2;
[vk::image_format("rgba32f")]
RWBuffer<float4> results;
[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID, uint3 dispatchID: SV_GroupID)
{
    uint index = threadId.x;
    results[index] = lerp(operands1[index],operands2[index],.5f);
}