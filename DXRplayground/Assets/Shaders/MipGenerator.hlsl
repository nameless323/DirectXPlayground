RWTexture2D<float4> Textures[10000] : register(u0, space2); // Mips array

struct CbTexData
{
    uint BaseMip;
    uint W; // [a_vorontcov] Maybe not?
    uint H;
};
ConstantBuffer<CbTexData> cbTexData : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID, uint2 indexInGroup : SV_GroupThreadID)
{
    float4 c0 = Textures[cbTexData.BaseMip][tId.xy * 2 + float2(0, 0)];
    float4 c1 = Textures[cbTexData.BaseMip][tId.xy * 2 + float2(0, 1)];
    float4 c2 = Textures[cbTexData.BaseMip][tId.xy * 2 + float2(1, 0)];
    float4 c3 = Textures[cbTexData.BaseMip][tId.xy * 2 + float2(1, 1)];
    float4 baseAvg = (c0 + c1 + c2 + c3) / 4.0f;

    Textures[cbTexData.BaseMip + 1][tId.xy] = baseAvg;
}