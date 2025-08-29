RWTexture2D target;

[shader("compute")]
[numthreads(16,16,1)]
void computeMain(uint3 threadId : SV_DispatchThreadID)
{
    uint2 size;
    target.GetDimensions(size.x, size.y);
    if(threadId.x < size.x && threadId.y < size.y)
    {
        float4 color;
        color.x=0;
        color.y=0;
        color.z=0;
        color.w=1;
        if(threadId.x !=0 && threadId.y !=0)
        {
            color.x = float(threadId.x)/float(size.x);
            color.y = float(threadId.y)/float(size.y);
        }
        target.Store(threadId.xy,color);
    }
}