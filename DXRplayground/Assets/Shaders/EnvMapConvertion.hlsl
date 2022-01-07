RWTexture2DArray<float4> cubemap : register(u0);
Texture2D<float4> envMap : register(t0);

SamplerState LinearClampSampler : register(s0);

struct CbData
{
    float4 eqMapCubeMapWH;
};
ConstantBuffer<CbData> cbData : register(b0);

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    uint iterCnt = cbData.eqMapCubeMapWH.z / 32;
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
    for (uint i = 0; i < iterCnt; ++i)
    {
        float2 uv = tId.xy / cbData.eqMapCubeMapWH.xy;
        cubemap[float3(tId.xyz)] = envMap.Sample(LinearClampSampler, uv);
    }
}