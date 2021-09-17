RWTexture2D<float4> Textures[10000] : register(u0, space2); // Mips array

struct CbTexData
{
    uint BaseMip;
    uint W;
    uint H;
    uint NumMips;
};
ConstantBuffer<CbTexData> cbTexData : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID, uint indexInGroup : SV_GroupIndex)
{
    if (tId.x > cbTexData.W || tId.y > cbTexData.H)
        return;
    float4 colors[4] = { float4(1, 0, 0, 1), float4(0, 1, 0, 1), float4(0, 0, 1, 1), float4(1, 0, 1, 0) };
    for (int i = 0; i < cbTexData.NumMips; ++i)
    {
        Textures[cbTexData.BaseMip + i][tId.xy / pow(2, i).xx] = colors[i];
    }
}