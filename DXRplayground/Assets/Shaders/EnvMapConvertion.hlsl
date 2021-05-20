RWStructuredBuffer<int> buf : register(u0);

[numthreads(10, 1, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    buf[tId.x] = tId.x;
}