Texture2D<float4> Textures[10000] : register(t0); // Mips array

struct CbTexData
{
    uint BaseMip;
    uint W;
    uint H;
    uint NumMips;
};
ConstantBuffer<CbTexData> cbTexData : register(b0);

groupshared float4 texData[32 * 32 * 1];

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID, uint indexInGroup : SV_GroupIndex)
{
    if (tId.x > W || tId.y > H)
        return;
    texData[indexInGroup] = Textures[cbTexData.BaseMip].Load(float3(tId.xy, 0));
    GroupMemoryBarrierWithGroupSync();
    uint currPatternOffset = 2;
    for (uint i = 0; i < cbTexData.NumMips; ++i)
    {
        if (tId.x % currPatternOffset == 0 && tId.y % currPatternOffset == 0)
        {
            float4 sample_lt = texData[tId.x + tId.y * 32];
            float4 sample_rt = texData[(tId.x + 1) + tId.y * 32];
            float4 sample_lb = texData[tId.x + (tId.y + 1) * 32];
            float4 sample_rb = texData[(tId.x + 1) + (tId.y + 1) * 32];
            float4 res = sample_lt + sample_rt + sample_lb + sample_rb;
            res /= 4.0f;
            Textures[cbTexData.BaseMip + i + 1] = res;
            sample_lt[tId.x + tId.y * 32] = res;
        }
        currPatternOffset *= 2;
        GroupMemoryBarrierWithGroupSync();
    }
}