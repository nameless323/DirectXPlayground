RWTexture2DArray<float4> cubemap : register(u0);
Texture2D<float4> envMap : register(t0);

#define PI 3.14159265358979323846f

SamplerState LinearClampSampler : register(s0);

struct CbData
{
    float4 eqMapCubeMapWH;
};
ConstantBuffer<CbData> cbData : register(b0);

float3 OutImgToXYZ(uint i, uint j, uint face, uint edge)
{
    float a = 2.0f * float(i) / float(edge);
    float b = 2.0f * float(j) / float(edge);
    float3 res;
    if (face == 0) // back
        res = float3(-1.0f, 1.0f - a, 3.0f - b);
    else if (face == 1)
        res = float3(a - 3.0f, -1.0f, 3.0f - b);
    else if (face == 2)
        res = float3(1.0f, a - 5.0f, 3.0f - b);
    else if (face == 3)
        res = float3(7.0f - a, 1.0f, 3.0f - b);
    else if (face == 4)
        res = float3(b - 1.0f, a - 5.0f, 1.0f);
    else
        res = float3(5.0f - b, a - 5.0f, -1.0f);
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    uint2 inSize = cbData.eqMapCubeMapWH.xy;
    uint2 edge = cbData.eqMapCubeMapWH.zw;
    uint face = uint(tId.z);
    float3 coord = OutImgToXYZ(tId.x, tId.y, face, edge);
    float theta = atan2(coord.y, coord.x);
    float r = length(coord.xy);
    float phi = atan2(coord.z, r);

    // to rasterizer
    float uf = 2.0f * edge.x * (theta + PI) / PI;
    float vf = 2.0f * edge.y * (PI / 2.0f - phi) / PI;
    float2 uv = float2(uf % inSize.x, clamp(vf, 0.0f, inSize.y - 1.0f));
    float4 color = envMap.Sample(LinearClampSampler, uv);
    cubemap[float3(tId.xyz)] = color;
/*
    uint u1 = floor(uf);
    uint v1 = floor(vf);
    uint u2 = u1 + 1;
    uint v2 = v1 + 1;
    float mu = uf - u1;
    float nu = vf - v1;

    float4 a = envMap.Load(float2(u1 % inSize.x, clamp(v1, 0, inSize.y - 1))); // don't need this shit. sample uf vf directly
    float4 b = envMap.Load(float2(u2 % inSize.x, clamp(v1, 0, inSize.y - 1)));
    float4 c = envMap.Load(float2(u1 % inSize.x, clamp(v2, 0, inSize.y - 1)));
    float4 d = envMap.Load(float2(u2 % inSize.x, clamp(v2, 0, inSize.y - 1)));

    float4 lh = lerp(a, b, mu);
    float4 ll = lerp(c, d, mu);
    float4 res = lerp(lh, ll, nu);

    //for (uint i = 0; i < iterCnt; ++i)
    {
        float2 uv = tId.xy / cbData.eqMapCubeMapWH.zw;
        cubemap[float3(tId.xyz)] = envMap.Sample(LinearClampSampler, uv);
    }*/
}