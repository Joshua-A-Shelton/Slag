uniform DescriptorHandle<RWStructuredBuffer<uint32_t>> buffer;

[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID, uint3 dispatchID: SV_GroupID)
{
    uint32_t index = threadId.x;
    buffer[index] = uint32_t(dispatchID.x);
}