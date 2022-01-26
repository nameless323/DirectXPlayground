RWTexture2DArray<float4> irrMap : register(u0);
TextureCube<float4> envMap : register(t0);

SamplerState LinearClampSampler : register(s0);

#define PI 3.14159265358979323846f
#define PI_2 (PI / 2.0f)

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
    return res;
}

[numthreads(32, 32, 1)]
void cs(uint3 tId : SV_DispatchThreadID)
{
    float3 N = normalize(RemapToXYZ(tId));
    N.y *= -1.0f;
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float ndu = dot(N, up);

    if (ndu == 1.0f || ndu == -1.0f)
        up = float3(1.0f, 0.0f, 0.0f);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float delta = 0.025f;
    float numSamples = 0.0f;

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);
    for (float phi = 0.0f; phi < PI * 2.0f; phi += delta)
    {
        for (float theta = 0.0f; theta < PI_2; theta += delta)
        {
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 sampleW = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; // yeah to matrix

            irradiance += envMap.SampleLevel(LinearClampSampler, sampleW, 0).rgb * cos(theta) * sin(theta);
            ++numSamples;
        }
    }

    irradiance = PI * irradiance / numSamples;
    irrMap[tId] = float4(irradiance, 1.0f); //envMap.SampleLevel(LinearClampSampler, N, 0);
    //irrMap[tId] = float4(N, 1);
}