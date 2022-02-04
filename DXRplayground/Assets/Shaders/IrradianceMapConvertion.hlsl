RWTexture2DArray<float4> irrMap : register(u0);
TextureCube<float4> envMap : register(t0);

SamplerState LinearClampSampler : register(s0);

#define PI 3.14159265358979323846f
#define PI_2 (PI / 2.0f)

struct Data
{
    float2 sizeDstSrc;
};
ConstantBuffer<Data> data : register(b0);

float3 RemapToXYZ(uint3 tId, float size)
{
    float2 coord = tId.xy / size;
    uint face = uint(tId.z);
    coord = coord * 2.0f - 1.0f;

    float3 res;
    if (face == 0)
        res = float3(1.0f, coord.y, -coord.x);
    else if (face == 1)
        res = float3(-1.0f, coord.yx);
    else if (face == 2)
        res = float3(coord.x, -1.0f, coord.y);
    else if (face == 3)
        res = float3(coord.x, 1.0f, -coord.y);
    else if (face == 4)
        res = float3(coord.xy, 1.0f);
    else
        res = float3(-coord.x, coord.y, -1.0f);
    res.y *= -1.0f;
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    float3 N = normalize(RemapToXYZ(tId, data.sizeDstSrc.x));
    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    float w = 0;
    for (uint i = 0; i < data.sizeDstSrc.y; ++i)
    {
        for (uint j = 0; j < data.sizeDstSrc.y; ++j)
        {
            for (uint f = 0; f < 6; ++f)
            {
                float3 dir = normalize(RemapToXYZ(uint3(i, j, f), data.sizeDstSrc.y));
                float dirDotN = dot(N, dir);
                if (dirDotN <= 0.0)
                    continue;
                irradiance += envMap.SampleLevel(LinearClampSampler, dir, 0).rgb * dirDotN;
                w += dirDotN;
            }
        }
    }
    irradiance /= w;
    irrMap[tId] = float4(irradiance, 1.0f);// envMap.SampleLevel(LinearClampSampler, N, 0).rgba;// float4(irradiance, 1.0f); //envMap.SampleLevel(LinearClampSampler, N, 0);
    //irrMap[tId] = float4(N, 1);
}