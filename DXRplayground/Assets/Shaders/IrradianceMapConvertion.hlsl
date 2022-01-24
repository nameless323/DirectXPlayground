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
    coord = coord * 0.5f - 0.5f;
    if (face == 0) // right
        res = float3(-0.5f, coord);
    else if (face == 1) // left
        res = float3(0.5, coord);
    else if (face == 2) // top
        res = float3(coord.x, 0.5f, coord.y);
    else if (face == 3) // bottom
        res = float3(coord.x, -0.5f, coord.y);
    else if (face == 4) // back
        res = float3(coord, -0.5f);
    else // fwd
        res = float3(coord, 0.5);
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    float3 N = normalize(RemapToXYZ(tId));
    irrMap[tId] = envMap.SampleLevel(LinearClampSampler, N, 0);
}