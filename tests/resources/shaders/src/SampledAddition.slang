uniform uint arrayindex;
uniform uint arrayLength;
SamplerState sampler;
[vk::image_format("rgba32f")]
Buffer<float4> operands1[2];
[vk::image_format("rgba8ui")]
Buffer<uint8_t4> operands2[2];
[vk::image_format("rgba32f")]
RWBuffer<float4> destination[2];
RWStructuredBuffer<float> storage[2];

[shader("compute")]
[numthreads(1,1,1)]
void main(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    float sampleIndex = (float)index/(float)arrayLength;
    float4 first = operands1[arrayindex].Sample(sampler,sampleIndex);
    uint8_t4 second = operands2[arrayindex].Sample(sampler,sampleIndex);
    float4 end = float4(first.x+second.x/255.0f,first.y+second.y/255.0f,first.z+second.z/255.0f,first.w+second.w/255.0f);
    destination[arrayindex][index] = end;
    storage[arrayindex][index] = index;
}