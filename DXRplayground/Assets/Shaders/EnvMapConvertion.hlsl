RWTexture2DArray<float4> cubemap : register(u0);
Texture2D<float4> envMap : register(t0);

#define PI 3.14159265358979323846f
#define PI_2 (PI / 2.0f)

SamplerState LinearClampSampler : register(s0);

struct CbData
{
    float4 eqMapCubeMapWH;
};
ConstantBuffer<CbData> cbData : register(b0);

float3 OutImgToXYZ(uint i, uint j, uint face, uint2 edge)
{
    float a = 2.0f * float(i) / float(edge.x) - 1.0f;
    float b = 2.0f * float(j) / float(edge.y) - 1.0f;
    float3 res;
    if (face == 0) // right
        res = float3(-a, 1.0f, -b);
    else if (face == 1) // left
        res = float3(a, -1.0f, -b);
    else if (face == 2) // top
        res = float3(b, a, 1.0f);
    else if (face == 3) // bottom
        res = float3(-b, a, -1.0f);
    else if (face == 4) // back
        res = float3(1.0f, a, -b);
    else // fwd
        res = float3(-1.0f, -a, -b);
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    uint2 eqMapSize = cbData.eqMapCubeMapWH.xy;
    float3 coord = OutImgToXYZ(tId.x, tId.y, uint(tId.z), cbData.eqMapCubeMapWH.zw);

    float theta = atan2(coord.y, coord.x);
    float r = length(coord.xy);
    float phi = atan2(coord.z, r);

    float2 uv = float2((theta + PI) / PI * 0.5f, (PI_2 - phi) / PI);
    cubemap[float3(tId.xyz)] = envMap.SampleLevel(LinearClampSampler, uv, 0);
}