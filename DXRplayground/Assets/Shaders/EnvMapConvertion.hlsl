RWTexture2DArray<float4> cubemap : register(u0);
Texture2D<float4> envMap : register(t0);

struct CbTIndices
{
    uint CubemapIndex;
    uint EnvMapIndex;
};
ConstantBuffer<CbTIndices> cbTIndices : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    cubemap[float3(tId.xy / 512.0f, 0)] = float4(1, 0, 0, 1);
    cubemap[float3(tId.xy / 512.0f, 1)] = float4(0, 1, 0, 1);
    cubemap[float3(tId.xy / 512.0f, 2)] = float4(0, 0, 1, 1);
    cubemap[float3(tId.xy / 512.0f, 3)] = float4(1, 1, 0, 1);
    cubemap[float3(tId.xy / 512.0f, 4)] = float4(0, 0, 0, 1);
    cubemap[float3(tId.xy / 512.0f, 5)] = float4(1, 1, 1, 1);
}