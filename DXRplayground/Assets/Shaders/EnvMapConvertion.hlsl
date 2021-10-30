RWTexture2DArray<float4> cubemap : register(u0);
Texture2D<float4> envMap : register(t0);

struct CbTIndices
{
    float4 eqMapCubeMapWH;
};
ConstantBuffer<CbTIndices> cbTIndices : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    cubemap[float3(tId.xy, 0)] = float4(1, 0, 0, 1);
    cubemap[float3(tId.xy, 1)] = float4(0, 1, 0, 1);
    cubemap[float3(tId.xy, 2)] = float4(0, 0, 1, 1);
    cubemap[float3(tId.xy, 3)] = float4(1, 1, 0, 1);
    cubemap[float3(tId.xy, 4)] = float4(0, 0, 0, 1);
    cubemap[float3(tId.xy, 5)] = float4(1, 1, 1, 1);
}