StructuredBuffer<float> buffer0;
StructuredBuffer<float> buffer1;
RWStructuredBuffer<float> result;

[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID, uint3 dispatchID: SV_GroupID)
{
    uint index = threadId.x;
    result[index] = buffer0[index] + buffer1[index] + dispatchID.x;
}