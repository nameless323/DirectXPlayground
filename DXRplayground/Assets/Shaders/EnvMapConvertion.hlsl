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
    if (tId.z == 0)
        cubemap[float3(tId.xyz)] = float4(1, 0, 0, 1);
    else if (tId.z == 1)
        cubemap[float3(tId.xyz)] = float4(0, 1, 0, 1);
    else if (tId.z == 2)
        cubemap[float3(tId.xyz)] = float4(0, 0, 1, 1);
    else if (tId.z == 3)
        cubemap[float3(tId.xyz)] = float4(1, 1, 0, 1);
    else if (tId.z == 4)
        cubemap[float3(tId.xyz)] = float4(0, 0, 0, 1);
    else
        cubemap[float3(tId.xyz)] = float4(1, 1, 1, 1);
}