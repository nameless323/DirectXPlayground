RWTexture2DArray<float4> dst : register(u0);
Texture2DArray<float4> src : register(t0);


struct Data
{
    uint iterCount;
};
ConstantBuffer<Data> data : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID, uint3 gId :SV_GroupID)
{
    float4 res = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < data.iterCount; ++i)
    {
        for (uint j = 0; j < data.iterCount; ++j)
        {
            uint3 tCoord;
            tCoord.x = tId.x * data.iterCount + i;
            tCoord.y = tId.y * data.iterCount + j;
            tCoord.z = tId.z;
            res += src[tCoord];
        }
    }
    res /= float(data.iterCount * data.iterCount);
    dst[tId].xyz = res;
}