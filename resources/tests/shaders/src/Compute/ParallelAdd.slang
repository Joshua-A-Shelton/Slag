uniform DescriptorHandle<RWBuffer<uint32_t>> buffer1;
uniform DescriptorHandle<RWBuffer<uint32_t>> buffer2;
uniform DescriptorHandle<RWBuffer<uint32_t>> results;

[shader("compute")]
[numthreads(1,1,1)]
void computeMain(uint3 threadID: SV_DispatchThreadID)
{
    uint index = threadID.x;
    results[index] = buffer1[index] + buffer2[index];
}