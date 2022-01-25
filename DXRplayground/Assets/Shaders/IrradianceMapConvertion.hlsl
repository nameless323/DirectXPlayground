RWTexture2DArray<float4> irrMap : register(u0);
TextureCube<float4> envMap : register(t0);

SamplerState LinearClampSampler : register(s0);

struct Data
{
    float size;
};
ConstantBuffer<Data> data : register(b0);

float3 RemapToXYZ(uint3 tId)
{
    float2 coord = tId.xy / data.size;
    uint face = uint(tId.z);
    coord = coord * 2.0f - 1.0f;

    float3 res;
    if (face == 0)
        res = float3(1.0f, coord.yx);
    else if (face == 1)
        res = float3(-1.0f, coord.yx);
    else if (face == 2)
        res = float3(coord.x, -1.0f, coord.y);
    else if (face == 3)
        res = float3(coord.x, 1.0f, -coord.y);
    else if (face == 4)
        res = float3(coord.xy, 1.0f);
    else
        res = float3(coord.xy, -1.0f);
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    float3 N = normalize(RemapToXYZ(tId));
    N.y *= -1.0f;
    irrMap[tId] = envMap.SampleLevel(LinearClampSampler, N, 0);
    //irrMap[tId] = float4(N, 1);
}