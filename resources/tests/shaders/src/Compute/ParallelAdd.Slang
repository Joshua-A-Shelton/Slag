uniform DescriptorHandle<RWStructuredBuffer<uint32_t>> buffer1;
uniform DescriptorHandle<RWStructuredBuffer<uint32_t>> buffer2;
uniform DescriptorHandle<RWStructuredBuffer<uint32_t>> results;

[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID, uint3 dispatchID: SV_GroupID)
{
    uint32_t index = threadId.x;
    results[index] = buffer1[index] + buffer2[index];
}